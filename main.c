
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
  int x = *(unsigned int*)arg;
  while (true)
  {
    sleep(x);
  }
}

void task4(void * arg)
{
  int x = *(unsigned int*)arg;
  while (true)
  {
    sleep(x);
  }
}

void task5(void * arg)
{
  // This task should never be scheduled due to its low priority.
  assert(false);
}

int main()
{
  unsigned int task3Arg = 3;
  unsigned int task4Arg = 5;
  
  hardware_init();
  kernel_create_task(&task1, NULL, 10);
  kernel_create_task(&task2, NULL, 10);
  kernel_create_task(&task3, &task3Arg, 15);
  kernel_create_task(&task4, &task4Arg, 20);
  kernel_create_task(&task5, NULL, 5);
  kernel_main();
}
