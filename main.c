
#include "system.h"
#include "task.h"
#include "kernel.h"
#include "syscall.h"
#include "hardware.h"

#include <stdbool.h>
#include <string.h>
#include <assert.h>

void task1(void * arg);
void task2(void * arg);
void task3(void * arg);
void task4(void * arg);
void task5(void * arg);

void task1(void * arg)
{
  // TODO make this task turn LD3 on
  while (true)
  {
    for (int i = 0; i < 0x10000; ++i)
    {
      if ((i & 0xFF) == 0)
      {
        yield();
      }
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

void task3(void * arg)
{
  while (true)
  {
    sleep(3);
  }
}

void task4(void * arg)
{
  while (true)
  {
    sleep(5);
  }
}

void task5(void * arg)
{
  // This task should never be scheduled due to its low priority.
  assert(false);
}

int main()
{
  hardware_init();
  kernel_create_task(&task1, 10);
  kernel_create_task(&task2, 10);
  kernel_create_task(&task3, 15);
  kernel_create_task(&task4, 20);
  kernel_create_task(&task5, 5);
  kernel_main();
}
