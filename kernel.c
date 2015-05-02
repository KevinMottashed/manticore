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

#define MAX_TASKS    4

// The system clock is divided by 8 and drives the SysTick timer.
#define SYSTICK_RELOAD_MS (SYSTEM_CLOCK / 8 / 1000)

// The time slice in milliseconds
#define TIME_SLICE_MS (10)

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
  int active_task = task_count - 1;
  
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
      tasks[active_task].state = STATE_READY;
      break;
    case SYSCALL_SLEEP:
      tasks[active_task].state = STATE_SLEEP;
      tasks[active_task].sleep = syscallContext.sleep * SYSTICK_RELOAD_MS;
      break;
    default:
      assert(false);
    }
    syscallValue = SYSCALL_NONE;
    
    // Iterate through all the tasks. We need to find this information:
    // 1. The next task to run.
    // 2. The minimum sleep time.
    // 3. The number of ready tasks.
    // 4. The number of sleeping tasks.
    int next_task = -1;
    unsigned int sleep = 0xFFFFFF; // Counter is 24 bits.
    int ready_tasks = 0;
    int sleeping_tasks = 0;
    for (int i = 0, j = (active_task + 1) % task_count;
         i < task_count; 
         ++i, j = (j + 1) % task_count)
    {
      switch (tasks[j].state)
      {
      case STATE_READY:
        ++ready_tasks;
        if (next_task == -1)
        {
          // This is the next task that will be run.
          next_task = j;
        }
        break;
      case STATE_SLEEP:
        ++sleeping_tasks;
        sleep = MIN(sleep, tasks[j].sleep);
        break;
      case STATE_RUNNING: // No tasks are running. The kernel is running.
      default:
        assert(false);
      }
    }
    
    // We don't want to sleep for less than 1ms.
    // When the systick timer is enabled we don't want it to immediatly fire.
    // We need to make it back to the user task before the IRQ goes off.
    // This will result in sleep() calls lasting 1ms longer than expected.
    // The other option is to wake up early but I think that sleeping longer is better.
    // In my experience sleep() calls are used to wait at least x amount of time.
    // For example:
    // 1. Start an operation.
    // 2. sleep()
    // 3. Check the status. The operation should be done by now or it's failed.
    sleep = MAX(sleep, SYSTICK_RELOAD_MS);
    
    // We know which task will run next but we still need to reconfigure
    // the SysTick timer.
    if (ready_tasks == 1 && sleeping_tasks == 0)
    {
      // In this case we only have 1 running task and no tasks
      // that will autonomously wake up. We leave the SysTick timer
      // disabled because we will never need to schedule another task
      // unless a system call is done.
    }
    else if (ready_tasks == 1 && sleeping_tasks > 0)
    {
      // In this case we have a single running task but
      // we will need to wakeup for a sleeping task.
      SysTick->LOAD = sleep;
      SysTick->VAL = 0;
      SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
    }
    else if (ready_tasks > 1)
    {
      // In this case we have multiple ready/sleeping tasks. We will
      // trigger the SysTick for the time slice or earlier if a sleeping
      // task needs to wake up sooner.
      sleep = MIN(sleep, TIME_SLICE_MS * SYSTICK_RELOAD_MS);
      SysTick->LOAD = sleep;
      SysTick->VAL = 0;
      SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
    }
    else if (ready_tasks == 0 && sleeping_tasks > 0)
    {
      // We only have sleeping tasks. In this case we will wakeup for 
      // the earliest task.
      SysTick->LOAD = sleep;
      SysTick->VAL = 0;
      SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
      
      // We have no tasks to schedule so we wait for the next SysTick interrupt.
      // The SysTick handler will run but will immediatly return.
      // See the comment in the SysTick handler.
      // Some user ISRs could run here. Hence the loop.
      do
      {
        __WFI();
      } while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0);
      continue;
    }
    else // ready_tasks == 0 && sleeping_tasks == 0
    {
      // I can't think of any cases where this isn't an error.
      assert(false);
    }
    
    // Move on to the next task
    active_task = next_task;
    tasks[next_task].state = STATE_RUNNING;
    savedContext = &tasks[next_task].context;
    ContextSwitch(&tasks[next_task].context);
  }
}

void kernel_create_task(task t)
{
  int i = task_count;
  tasks[i].state = STATE_READY;
  tasks[i].context.PC = (uint32_t)t;
  tasks[i].context.SP = (uint32_t)(tasks[i].stack + sizeof(tasks[i].stack)/8 - 1);
  ++task_count;
}
