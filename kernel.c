#include "kernel.h"

#include "task.h"
#include "system.h"
#include "syscall.h"
#include "utils.h"
#include "hardware.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

void ContextSwitch(volatile context_t * context);

#define MAX_TASKS (8)

// The system clock is divided by 8 and drives the SysTick timer.
#define SYSTICK_RELOAD_MS (SYSTEM_CLOCK / 8 / 1000)

 // The counter is 24 bits.
#define MAX_SYSTICK_RELOAD (0xFFFFFF)

// The time slice in milliseconds
#define TIME_SLICE_MS (10)

// The number of SysTick ticks in a time slice
#define TIME_SLICE_TICKS (TIME_SLICE_MS * SYSTICK_RELOAD_MS)

int task_count = 0;

volatile task_t tasks[MAX_TASKS];
volatile context_t * savedContext;
volatile uint8_t syscallValue;
context_t kernelContext;

void kernel_main(void)
{
  // Enable SysTick IRQ.
  SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
  
  // Setting this value will result in task 1 going first.
  int activeTask = task_count - 1;
  
  while (true)
  {
    // Find out how much time passed since the kernel last ran.
    uint32_t load = SysTick->LOAD;
    uint32_t value;
    if (syscallValue == SYSCALL_NONE)
    {
      // We need to check the syscall value.
      // The systick timer is automatically reloaded so if
      // we got here it means that the timer reached zero.
      value = 0;
    }
    else
    {
      value = SysTick->VAL;
    }
    uint32_t delta = load - value;

    // Iterate through all the tasks to update the sleep time left.  
    for (int i = 0; i < task_count; ++i)
    {
      if (tasks[i].state == STATE_SLEEP)
      {
        if (delta >= tasks[i].sleep)
        {
          tasks[i].sleep = 0;
          tasks[i].state = STATE_READY;
        }
        else
        {
          tasks[i].sleep -= delta;
        }
      }
    }
    
    // Handle the system call.
    switch (syscallValue)
    {
    case SYSCALL_NONE:
    case SYSCALL_YIELD:
      // There's nothing special to do here. One of the following occured:
      // 1. The time slice expired.
      // 2. A task yielded the remainder of its time slice.
      // Either way, the current task is no longer running.
      tasks[activeTask].state = STATE_READY;
      break;
    case SYSCALL_SLEEP:
      tasks[activeTask].state = STATE_SLEEP;
      tasks[activeTask].sleep = syscallContext.sleep * SYSTICK_RELOAD_MS;
      break;
    default:
      assert(false);
    }
    syscallValue = SYSCALL_NONE;
    
    // We will iterate through all the tasks twice.
    // The first pass will determine which task to run next.
    // The second pass will determine if the task will ever need to yield to
    // a sleeping task. This is a priority round-robin scheduler. The highest
    // priority ready task will always run next and will never yield to
    // a lower priority task. When counting ready tasks we only care about
    // those at the highest priority because the lower priority ones will
    // never be scheduled unless a system call changes the state of the system.
    
    // First pass. Find the following information:
    // 1. The next task to run.
    // 2. The highest priority among the ready tasks.
    // 3. The number of ready tasks with the highest priority.
    int nextTask = -1;
    int readyTasks = 0;
    int highPriority = -1; // The highest priority of all READY tasks
    for (int i = 0, j = (activeTask + 1) % task_count;
         i < task_count; 
         ++i, j = (j + 1) % task_count)
    {
      switch (tasks[j].state)
      {
      case STATE_READY:
        if (tasks[j].priority > highPriority)
        {
          nextTask = j;
          highPriority = tasks[j].priority;
          readyTasks = 1;
        }
        else if (tasks[j].priority == highPriority)
        {
          ++readyTasks;
        }
        break;
      case STATE_SLEEP:
        // The second iteration will deal with the sleeping tasks.
        break;
      case STATE_RUNNING: // No tasks are running. The kernel is running.
      default:
        assert(false);
      }
    }
    
    // We now know the next task that will run. We need to find out if
    // there's a sleeping task that needs to wakeup before the time slice expires.
    // Second pass. Find the following information:
    // 1. The length of time to sleep for.
    // 2. The number of sleeping tasks of equal or higher priority.
    unsigned int sleep = MAX_SYSTICK_RELOAD;
    int sleepingTasks = 0;
    if (readyTasks > 0)
    {
      if (readyTasks > 1)
      {
        // There's more than 1 task that needs to run.
        // We'll need to context switch after a time slice.
        sleep = TIME_SLICE_TICKS;
      }
      for (int i = 0; i < task_count; ++i)
      {
        if (tasks[i].state == STATE_SLEEP)
        {
          if (tasks[i].priority > highPriority)
          {
            // A higher priority sleeping task can reduce the sleep time
            // to less than the time slice.
            sleep = MIN(sleep, tasks[i].sleep);
            ++sleepingTasks;
          }
          else if (tasks[i].priority == highPriority)
          {
            // An equal priority sleeping task can reduce the sleep time
            // but still needs to respect the time slice.
            unsigned int temp = MIN(sleep, tasks[i].sleep);
            sleep = MAX(temp, TIME_SLICE_TICKS);
            ++sleepingTasks;
          }
        }
      }
    }
    
    // We don't want to sleep for less than 1ms.
    // When the systick timer is enabled we don't want it to immediatly fire.
    // We need to make it back to the user task before the IRQ goes off.
    // This will result in sleep() calls lasting up to 1ms longer than expected.
    // The other option is to wake up early but I think that sleeping longer is better.
    // In my experience sleep() calls are used to wait at least x amount of time.
    // For example:
    // 1. Start an operation.
    // 2. sleep()
    // 3. Check the status. The operation should be done by now or its failed.
    sleep = MAX(sleep, SYSTICK_RELOAD_MS);
    
    // Restart the systick timer.
    SysTick->LOAD = sleep;
    SysTick->VAL = 0;
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
    
    if (readyTasks > 0)
    {
      // We have a task to schedule.
      activeTask = nextTask;
      tasks[nextTask].state = STATE_RUNNING;
      savedContext = &tasks[nextTask].context;
      ContextSwitch(&tasks[nextTask].context);
    }
    else
    {
      // We have no tasks to schedule so we wait for the next SysTick interrupt.
      // The SysTick handler will run but will immediatly return.
      // See the comment in the SysTick handler.
      // Some user ISRs could run here. Hence the loop.
      // This loop is essentially the idle task.
      do
      {
        __WFI();
      } while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0);
    }
  }
}

void kernel_create_task(task t, uint8_t priority)
{
  int i = task_count;
  tasks[i].state = STATE_READY;
  tasks[i].priority = priority;
  tasks[i].context.PC = (uint32_t)t;
  tasks[i].context.SP = (uint32_t)(tasks[i].stack + sizeof(tasks[i].stack)/8 - 1);
  ++task_count;
}
