/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <kevinmottashed@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * -Kevin Mottashed
 * ----------------------------------------------------------------------------
 */

#include "manticore.h"
#include "clock.h"
#include "gpio.h"
#include "utils.h"
#include "tests.h"

#include <stdbool.h>
#include <string.h>
#include <assert.h>

static __task void * task_run_all_tests(void * arg);

// ARM requires 8 byte alignment for the stack.
#define STACK_SIZE (256)
#pragma data_alignment = 8
static uint8_t stack[STACK_SIZE];

__task void * task_run_all_tests(void * arg)
{
  // Turn the green LED on when the tests pass the first time.
  // Toggle the blue LED everytime they pass after that.
  test_run_all();
  gpio_led3_on();
  while (true)
  {
    test_run_all();
    gpio_led4_on();
    test_run_all();
    gpio_led4_off();
  }
}

int main()
{
  // Initialize the hardware.
  clock_init();
  gpio_init();

  // Initialize the kernel
  manticore_init();

  // Create the task that will drive the tests.
  struct task task;
  task_init(&task, task_run_all_tests, NULL, stack, STACK_SIZE, 10);

  // Start the kernel.
  manticore_main();
}
