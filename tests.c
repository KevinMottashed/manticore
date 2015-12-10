/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <kevinmottashed@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * -Kevin Mottashed
 * ----------------------------------------------------------------------------
 */

#include "tests.h"

#include "manticore.h"

#include <assert.h>

extern __task void * Task_Busy_Yield(void * arg);
static __task void * task_test_get_priority(void * arg);
static __task void * task_test_sleep(void * arg);
static __task void * task_test_delay(void * arg);
static __task void * task_test_yield(void * arg);

static void test_context_switching(void);
static void test_get_priority(void);
static void test_sleep(void);
static void test_delay(void);
static void test_yield(void);

// 128 bytes of stack should be enough for these dummy tasks.
#define NUM_TASKS (3)
#define STACK_SIZE (128)

// ARM requires 8 byte alignment for the stack.
#pragma data_alignment = 8
static uint8_t stacks[NUM_TASKS][STACK_SIZE];

void test_run_all(void)
{
  test_context_switching();
  test_get_priority();
  test_sleep();
  test_delay();
  test_yield();
}

void test_context_switching(void)
{
  task_create(Task_Busy_Yield, NULL, stacks[0], STACK_SIZE, 10);
  task_create(Task_Busy_Yield, NULL, stacks[1], STACK_SIZE, 10);
  task_wait(NULL);
  task_wait(NULL);
}

__task void * task_test_get_priority(void * arg)
{
  uint8_t priority = (uint32_t)arg;
  assert(priority == task_get_priority(NULL));
  return NULL;
}

void test_get_priority(void)
{
  for (uint8_t priority = 1; priority < 100; ++priority)
  {
    task_create(task_test_get_priority, (void*)priority, stacks[0], STACK_SIZE, priority);
    task_wait(NULL);
  }
}

__task void * task_test_sleep(void * arg)
{
  uint32_t duration = (uint32_t)arg;
  task_sleep(duration);
  return NULL;
}

void test_sleep(void)
{
  // Create 2 tasks that sleep. Make sure they terminate in the right order.
  task_handle_t t1 = task_create(task_test_sleep, (void*)1, stacks[0], STACK_SIZE, 10);
  task_handle_t t2 = task_create(task_test_sleep, (void*)2, stacks[1], STACK_SIZE, 10);
  task_handle_t ret1 = NULL;
  task_handle_t ret2 = NULL;
  task_wait(&ret1);
  task_wait(&ret2);
  assert(ret1 == t1);
  assert(ret2 == t2);
}

__task void * task_test_delay(void * arg)
{
  uint32_t duration = (uint32_t)arg;
  task_delay(duration);
  return NULL;
}

void test_delay(void)
{
  // Create 2 tasks that sleep. Make sure they terminate in the right order.
  task_handle_t t1 = task_create(task_test_delay, (void*)10, stacks[0], STACK_SIZE, 10);
  task_handle_t t2 = task_create(task_test_delay, (void*)20, stacks[1], STACK_SIZE, 10);
  task_handle_t ret1 = NULL;
  task_handle_t ret2 = NULL;
  task_wait(&ret1);
  task_wait(&ret2);
  assert(ret1 == t1);
  assert(ret2 == t2);
}

__task void * task_test_yield(void * arg)
{
  // This test works by having 2 tasks running this function.
  // Each task flips the boolean and yields to the other one.
  // Each task should see the bool return to its original value after yielding.
  volatile bool * value = (volatile bool*)arg;
  for (int32_t i = 0; i < 1000; ++i)
  {
    bool prev_value = *value;
    *value = !*value;
    task_yield();
    assert(*value == prev_value);
  }
  *value = !*value;
  return NULL;
}

void test_yield(void)
{
  bool value = true;
  task_handle_t t1 = task_create(task_test_yield, &value, stacks[0], STACK_SIZE, 10);
  task_handle_t t2 = task_create(task_test_yield, &value, stacks[1], STACK_SIZE, 10);
  task_wait(NULL);
  task_wait(NULL);
}
