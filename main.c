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

#include <stdbool.h>
#include <string.h>
#include <assert.h>

static void task1(void * arg);
static void task2(void * arg);
static void task3(void * arg);
static void task4(void * arg);
static void task5(void * arg);

static uint8_t stack1[256];
static uint8_t stack2[192];
static uint8_t stack3[128];
static uint8_t stack4[147];
static uint8_t stack5[187];

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
  unsigned int x = *(unsigned int*)arg;
  while (true)
  {
    sleep(x);
  }
}

void task4(void * arg)
{
  unsigned int x = *(unsigned int*)arg;
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
  kernel_create_task(&task1, NULL, stack1, sizeof(stack1), 10);
  kernel_create_task(&task2, NULL, stack2, sizeof(stack2), 10);
  kernel_create_task(&task3, &task3Arg, stack3, sizeof(stack3), 15);
  kernel_create_task(&task4, &task4Arg, stack4, sizeof(stack4), 20);
  kernel_create_task(&task5, NULL, stack5, sizeof(stack5), 5);
  kernel_main();
}
