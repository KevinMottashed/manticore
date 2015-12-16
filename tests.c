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
static __task void * task_test_mutex_lock(void * arg);
static __task void * task_test_mutex_trylock(void * arg);

static void test_context_switching(void);
static void test_get_priority(void);
static void test_sleep(void);
static void test_delay(void);
static void test_yield(void);
static void test_mutex_lock(void);
static void test_mutex_trylock(void);

// 128 bytes of stack should be enough for these dummy tasks.
#define NUM_TASKS (8)
#define STACK_SIZE (128)

// ARM requires 8 byte alignment for the stack.
#pragma data_alignment = 8
static uint8_t stacks[NUM_TASKS][STACK_SIZE];
static struct task tasks[NUM_TASKS];

void test_run_all(void)
{
  test_context_switching();
  test_get_priority();
  test_sleep();
  test_delay();
  test_yield();
  test_mutex_lock();
  test_mutex_trylock();
}

void test_context_switching(void)
{
  task_init(&tasks[0], Task_Busy_Yield, NULL, stacks[0], STACK_SIZE, 10);
  task_init(&tasks[1], Task_Busy_Yield, NULL, stacks[1], STACK_SIZE, 10);
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
    task_init(&tasks[0], task_test_get_priority, (void*)priority, stacks[0], STACK_SIZE, priority);
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
  // Create tasks that sleep. Make sure they terminate in the right order.
  // Only use 3 tasks for this test so that it doesn't take longer than 3 seconds.
  for (int32_t i = 0; i < 3; ++i)
  {
    task_init(&tasks[i], task_test_sleep, (void*)i, stacks[i], STACK_SIZE, 5);
  }
  for (int32_t i = 0; i < 3; ++i)
  {
    struct task * ret = NULL;
    task_wait(&ret);
    assert(ret == &tasks[i]);
  }
}

__task void * task_test_delay(void * arg)
{
  uint32_t duration = (uint32_t)arg;
  task_delay(duration);
  return NULL;
}

void test_delay(void)
{
  // Create tasks that sleep. Make sure they terminate in the right order.
  for (int32_t i = 0; i < NUM_TASKS; ++i)
  {
    task_init(&tasks[i], task_test_delay, (void*)(i * 10), stacks[i], STACK_SIZE, 5);
  }
  for (int32_t i = 0; i < NUM_TASKS; ++i)
  {
    struct task * ret = NULL;
    task_wait(&ret);
    assert(ret == &tasks[i]);
  }
}

__task void * task_test_yield(void * arg)
{
  // This test works by having multiple tasks running this function.
  // Each task increments the number and yields to the other ones.
  // Each task should see the number incremented by all the other tasks.
  volatile int32_t * value = (volatile int32_t *)arg;
  for (int32_t i = 0; i < 1000; ++i)
  {
    int32_t prev_value = *value;
    *value += 1;
    task_yield();
    assert(*value == prev_value + NUM_TASKS);
  }
  *value += 1;
  return NULL;
}

void test_yield(void)
{
  int32_t value = 0;
  for (int32_t i = 0; i < NUM_TASKS; ++i)
  {
    task_init(&tasks[i], task_test_yield, &value, stacks[i], STACK_SIZE, 5);
  }
  for (int32_t i = 0; i < NUM_TASKS; ++i)
  {
    struct task * ret = NULL;
    task_wait(&ret);
    assert(ret == &tasks[i]);
  }
}

struct test_mutex_data
{
  struct mutex mutex;
  volatile bool critical_section;
};

static __task void * task_test_mutex_lock(void * arg)
{
  struct test_mutex_data * data = (struct test_mutex_data *)arg;
  for (int32_t i = 0; i < 10; ++i)
  {
    mutex_lock(&data->mutex);
    assert(!data->critical_section);
    data->critical_section = true;
    task_yield();
    task_delay(20);
    assert(data->critical_section);
    data->critical_section = false;
    mutex_unlock(&data->mutex);
  }
  return NULL;
}

static void test_mutex_lock(void)
{
  struct test_mutex_data data;
  mutex_init(&data.mutex);
  data.critical_section = false;
  for (int32_t i = 0; i < NUM_TASKS; ++i)
  {
    task_init(&tasks[i], task_test_mutex_lock, &data, stacks[i], STACK_SIZE, 5);
  }
  for (int32_t i = 0; i < NUM_TASKS; ++i)
  {
    struct task * ret = NULL;
    task_wait(&ret);
    assert(ret == &tasks[i]);
  }
}

static __task void * task_test_mutex_trylock(void * arg)
{
  struct test_mutex_data * data = (struct test_mutex_data *)arg;
  for (int32_t i = 0; i < 10; ++i)
  {
    if (!mutex_trylock(&data->mutex))
    {
      continue;
    }
    assert(!data->critical_section);
    data->critical_section = true;
    task_yield();
    task_delay(20);
    assert(data->critical_section);
    data->critical_section = false;
    mutex_unlock(&data->mutex);
  }
  return NULL;
}

static void test_mutex_trylock(void)
{
  struct test_mutex_data data;
  mutex_init(&data.mutex);
  data.critical_section = false;
  for (int32_t i = 0; i < NUM_TASKS; ++i)
  {
    task_init(&tasks[i], task_test_mutex_trylock, &data, stacks[i], STACK_SIZE, 5);
  }

  // In this test the tasks aren't guaranteed to terminate in the same order.
  // The tasks waiting for the mutex spin instead of queuing up.
  // The first task that didn't waste its time slice spinning will get the mutex.
  for (int32_t i = 0; i < NUM_TASKS; ++i)
  {
    task_wait(NULL);
  }
}
