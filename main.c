
#include "system.h"
#include "task.h"
#include "kernel.h"

#include <stdbool.h>
#include <string.h>

void task1(void * arg);
void task2(void * arg);

void task1(void * arg)
{
  // TODO make this task turn LD3 on
  while (true)
  {
    for (int i = 0; i < 0x10000; ++i)
    {    
    }
  }
}

void task2(void * arg)
{
  // TODO make this task turn LD3 off
  while (true)
  {
    for (int i = 0x10000; i < 0x20000; ++i)
    {
    }
  }
}

int main()
{
  kernel_create_task(&task1);
  kernel_create_task(&task2);
  kernel_main();
}
