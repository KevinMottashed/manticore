/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <kevinmottashed@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return. 
 * -Kevin Mottashed
 * ----------------------------------------------------------------------------
 */

#include "system.h"
#include "task.h"
#include "kernel.h"
#include "syscall.h"
#include "hardware.h"
#include "mutex.h"

#include <stdbool.h>
#include <string.h>
#include <assert.h>

static __task void task1(void * arg);
static __task void task2(void * arg);
static __task void task3(void * arg);
static __task void task4(void * arg);
static __task void task5(void * arg);
static __task void task6(void * arg);

static uint8_t stack1[256];
static uint8_t stack2[192];
static uint8_t stack3[128];
static uint8_t stack4[147];
static uint8_t stack5[187];
static uint8_t stack6[128];

static mutex_t m1;

__task void task1(void * arg)
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

__task void task2(void * arg)
{
  // TODO make this task turn LD3 off
  while (true)
  {
    for (int i = 0x10000; i < 0x20000; ++i)
    {
      if (mutex_trylock(&m1))
      {
        mutex_unlock(&m1);
      }
      else
      {
        mutex_lock(&m1);
        mutex_unlock(&m1);
      }
    }
  }
}

__task void task3(void * arg)
{
  unsigned int x = *(unsigned int*)arg;
  while (true)
  {
    sleep(x);
  }
}

__task void task4(void * arg)
{
  unsigned int x = *(unsigned int*)arg;
  while (true)
  {
    sleep(x);
  }
}

__task void task5(void * arg)
{
  // This task should never be scheduled due to its low priority.
  assert(false);
}

__task void task6(void * arg)
{
  while (true)
  {
    mutex_lock(&m1);
    delay(50);
    mutex_unlock(&m1);
  }
}

int main()
{
  unsigned int task3Arg = 3;
  unsigned int task4Arg = 5;
  
  hardware_init();
  mutex_init(&m1);
  kernel_create_task(&task1, NULL, stack1, sizeof(stack1), 10);
  kernel_create_task(&task2, NULL, stack2, sizeof(stack2), 10);
  kernel_create_task(&task3, &task3Arg, stack3, sizeof(stack3), 15);
  kernel_create_task(&task4, &task4Arg, stack4, sizeof(stack4), 20);
  kernel_create_task(&task5, NULL, stack5, sizeof(stack5), 5);
  kernel_create_task(&task6, NULL, stack6, sizeof(stack6), 10);
  kernel_main();
}
