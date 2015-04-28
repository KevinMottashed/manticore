#include "kernel.h"

#include "task.h"
#include "system.h"
#include "syscall.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

void ContextSwitch(volatile context_t * context);

#define KERNEL_TASK -1
#define MAX_TASKS    2

// TODO: Configure clocks and calibrate this value
#define SYSTICK_RELOAD (6000)

int active_task = KERNEL_TASK;
int task_count = 0;

volatile task_t tasks[MAX_TASKS];
volatile context_t * savedContext;
volatile uint8_t syscallValue;
context_t kernelContext;

void kernel_main(void)
{
  SysTick_Config(SYSTICK_RELOAD);
  while (true)
  {
    switch (syscallValue)
    {
    case SYSCALL_NONE:
      // No syscall. We must have gotten here from the systick IRQ.
      break;
    case SYSCALL_YIELD:
      // The yield syscall causes the calling task to give up
      // it's remaining time slice. Reset and restart the system tick.
      SysTick->VAL = 0;
      SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
      break;
    default:
      assert(false);
    }
    syscallValue = SYSCALL_NONE;
    
    // Move on to the next task
    ++active_task;
    active_task %= task_count;
    savedContext = &tasks[active_task].context;
    ContextSwitch(&tasks[active_task].context);
  }
}

void kernel_create_task(task t)
{
  int i = task_count;
  tasks[i].context.PC = (uint32_t)t;
  tasks[i].context.SP = (uint32_t)(tasks[i].stack + sizeof(tasks[i].stack)/4 - 1);
  ++task_count;
}
