/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <kevinmottashed@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return. 
 * -Kevin Mottashed
 * ----------------------------------------------------------------------------
 */

#include "kernel.h"

#include "task.h"
#include "system.h"
#include "syscall.h"
#include "utils.h"
#include "hardware.h"
#include "mutex.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define MAX_TASKS (8)

// The system clock is divided by 8 and drives the SysTick timer.
#define SYSTICK_RELOAD_MS (SYSTEM_CLOCK / 8 / 1000)

 // The counter is 24 bits.
#define MAX_SYSTICK_RELOAD (0xFFFFFF)

// The time slice in milliseconds
#define TIME_SLICE_MS (10)

// The number of SysTick ticks in a time slice
#define TIME_SLICE_TICKS (TIME_SLICE_MS * SYSTICK_RELOAD_MS)

// This is the aproximate number of systicks spent in the kernel
// while the systick timer is being reset.
#define KERNEL_SYSTICK_DISABLE (2)

int task_count = 0;

volatile task_t tasks[MAX_TASKS];
volatile uint8_t syscallValue;
volatile uint32_t * savedStackPointer;

static void kernel_update_sleep(unsigned int ticks);
static void kernel_handle_syscall(uint8_t value, int activeTask);

void kernel_main(void)
{
  // Enable SysTick IRQ.
  SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
  
  // Setting this value will result in task 1 going first.
  // Assuming all tasks have equal priorities.
  int activeTask = -1;
  
  unsigned int kernelTicks = 0;
  unsigned int systickReload = 0xFFFFFF;
  
  while (true)
  {
    // The systick value when entering the kernel.
    int kernelStartTicks = SysTick->VAL;
    
    // Update how much time is left for each sleeping task to wake up.
    kernel_update_sleep(systickReload - kernelStartTicks + kernelTicks);
    
    // Handle the system call.
    kernel_handle_syscall(syscallValue, activeTask);
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
      case STATE_MUTEX:
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
    else
    {
      // We have no ready tasks.
      // Sleep until the next sleeping task needs wakes up.
      for (int i = 0; i < task_count; ++i)
      {
        if (tasks[i].state == STATE_SLEEP)
        {
          sleep = MIN(sleep, tasks[i].sleep);
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
    systickReload = sleep;
    
    // Remember the amount of time spent in the kernel.
    kernelTicks = kernelStartTicks - SysTick->VAL + KERNEL_SYSTICK_DISABLE;
    
    // Restart the systick timer.  
    SysTick->LOAD = systickReload;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_TICKINT_Msk;
    SysTick->LOAD = 0;
    
    if (readyTasks > 0)
    {
      // We have a task to schedule.
      activeTask = nextTask;
      tasks[nextTask].state = STATE_RUNNING;
      savedStackPointer = &tasks[nextTask].stackPointer;
      
      // Trigger a PendSV interrupt to return to task context.
      SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
    }
    else
    {
      // We have no tasks to schedule so we wait for the next SysTick interrupt.
      // The SysTick handler will run but will immediatly return.
      // See the comment in the SysTick handler.
      // Some user ISRs could run here. Hence the loop.
      // This loop is essentially the idle task.
      activeTask = -1;
      do
      {
        __WFI();
      } while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0);
    }
  }
}

void kernel_create_task(task t, void * arg, void * stack, uint32_t stackSize, uint8_t priority)
{
  assert(stackSize >= sizeof(context_t));
  
  context_t context;
  memset(&context, 0, sizeof(context)); // Most registers will start off as zero.
  context.R0 = (uint32_t)arg;
  context.PC = (uint32_t)t; // The program counter starts at the task entry point.
  context.xPSR.b.T = 1; // Enabled Thumb mode.
  
  int i = task_count;
  tasks[i].state = STATE_READY;
  tasks[i].priority = priority;
  tasks[i].stackPointer = (uint32_t)stack + stackSize;
  tasks[i].stackPointer &= ~0x7; // The stack must be 8 byte aligned.
  tasks[i].stackPointer -= sizeof(context_t);
  memcpy((void*)tasks[i].stackPointer, &context, sizeof(context));
  
  ++task_count;
}

void kernel_update_sleep(unsigned int ticks)
{
  // Iterate through all the tasks to update the sleep time left.
  for (int i = 0; i < task_count; ++i)
  {
    if (tasks[i].state == STATE_SLEEP)
    {
      if (ticks >= tasks[i].sleep)
      {
        tasks[i].sleep = 0;
        tasks[i].state = STATE_READY;
      }
      else
      {
        tasks[i].sleep -= ticks;
      }
    }
  }  
}

void kernel_handle_syscall(uint8_t value, int activeTask)
{
  switch (value)
  {
  case SYSCALL_NONE:
  case SYSCALL_YIELD:
    // There's nothing special to do here. One of the following occured:
    // 1. The time slice expired.
    // 2. A task yielded the remainder of its time slice.
    // 3. The systick expired but no tasks were running.
    // Either way, the active task is no longer running.
    if (activeTask != -1)
    {
      tasks[activeTask].state = STATE_READY;
    }
    break;
  case SYSCALL_SLEEP:
    tasks[activeTask].state = STATE_SLEEP;
    tasks[activeTask].sleep = syscallContext.sleep * SYSTICK_RELOAD_MS;
    break;
  case SYSCALL_MUTEX_LOCK:
    tasks[activeTask].state = STATE_MUTEX;
    tasks[activeTask].mutex = syscallContext.mutex;
    break;
  case SYSCALL_MUTEX_UNLOCK:
    {
      // A task is still ready after unlocking a mutex.
      tasks[activeTask].state = STATE_READY;
        
      // Find the next highest priority task that's waiting
      // for this mutex and unblock it.
      int nextTask = -1;
      int highPriority = -1;
      for (int i = 0, j = (activeTask + 1) % task_count;
           i < task_count; 
           ++i, j = (j + 1) % task_count)
      {
        if (tasks[j].state == STATE_MUTEX && 
            tasks[j].mutex == syscallContext.mutex &&
            tasks[j].priority > highPriority)
        {
          nextTask = j;
        }
      }
      if (nextTask != -1)
      {
        tasks[nextTask].state = STATE_READY;
        tasks[nextTask].mutex = NULL;
      }
    }
    break;
  default:
    assert(false);
  }  
}
