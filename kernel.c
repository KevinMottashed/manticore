#include "kernel.h"

#include "task.h"
#include "system.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

void ContextSwitch(context_t * context);

#define KERNEL_TASK -1
#define MAX_TASKS    2

int active_task = KERNEL_TASK;
int task_count = 0;

task_t tasks[MAX_TASKS];
context_t * savedContext;
context_t kernelContext;

void kernel_main(void)
{
  SysTick_Config(6000);
  while (true)
  {
    for (int i = 0; i < task_count; ++i)
    {
      active_task = i;
      savedContext = &tasks[i].context;
      ContextSwitch(&tasks[i].context);
    }
  }
}

void kernel_create_task(task t)
{
  int i = task_count;
  tasks[i].context.PC = (uint32_t)t;
  tasks[i].context.SP = (uint32_t)(tasks[i].stack + sizeof(tasks[i].stack)/4 - 1);
  ++task_count;
}
