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
#include "utils.h"

#include <assert.h>

// We need an assert macro that's compiled in release mode for unit testing.
// It would be nice to have a real unit testing framework but the code size
// is too much and there's no serial port to print results. The debugger
// automatically stops on failure in release mode. In debug mode, we get
// the usual IAR assert failure behavior with the popup.
#ifdef NDEBUG
#define ut_assert(x)\
  if (!(x)) {\
    BREAKPOINT;\
  }
#else
#define ut_assert assert
#endif

extern __task void * Task_Busy_Yield(void * arg);
static __task void * task_test_get_priority(void * arg);
static __task void * task_test_sleep(void * arg);
static __task void * task_test_delay(void * arg);
static __task void * task_test_yield(void * arg);
static __task void * task_test_mutex_lock(void * arg);
static __task void * task_test_mutex_trylock(void * arg);
static __task void * task_test_mutex_priority1_high(void * arg);
static __task void * task_test_mutex_priority1_med(void * arg);
static __task void * task_test_mutex_priorityl_low(void * arg);
static __task void * task_test_mutex_priority2_high(void * arg);
static __task void * task_test_mutex_priority2_low(void * arg);

static void test_context_switching(void);
static void test_get_priority(void);
static void test_sleep(void);
static void test_delay(void);
static void test_yield(void);
static void test_mutex_lock(void);
static void test_mutex_trylock(void);
static void test_mutex_priority1(void);
static void test_mutex_priority2(void);

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
  test_mutex_priority1();
  test_mutex_priority2();
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
  ut_assert(priority == task_get_priority(NULL));
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
    ut_assert(ret == &tasks[i]);
    ut_assert(ret->state == STATE_DEAD);
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
    ut_assert(ret == &tasks[i]);
    ut_assert(ret->state == STATE_DEAD);
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
    ut_assert(*value == prev_value + NUM_TASKS);
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
    ut_assert(ret == &tasks[i]);
    ut_assert(ret->state == STATE_DEAD);
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
    ut_assert(!data->critical_section);
    data->critical_section = true;
    task_yield();
    task_delay(20);
    ut_assert(data->critical_section);
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
    ut_assert(ret == &tasks[i]);
    ut_assert(ret->state == STATE_DEAD);
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
    ut_assert(!data->critical_section);
    data->critical_section = true;
    task_yield();
    task_delay(20);
    ut_assert(data->critical_section);
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

struct test_mutex_priority1_data
{
  struct task * high;
  struct task * med;
  struct task * low;
  struct mutex mutex;
};

static __task void * task_test_mutex_priority1_high(void * arg)
{
  struct test_mutex_priority1_data * data = (struct test_mutex_priority1_data *)arg;

  ut_assert(data->low->state == STATE_READY);
  ut_assert(data->med->state == STATE_READY);
  ut_assert(data->low->priority == data->low->provisioned_priority);
  ut_assert(data->med->priority == data->med->provisioned_priority);
  ut_assert(data->high->priority == data->high->provisioned_priority);

  task_delay(20);
  mutex_lock(&data->mutex);

  ut_assert(data->low->state == STATE_READY);
  ut_assert(data->med->state == STATE_MUTEX);
  ut_assert(data->low->priority == data->low->provisioned_priority);
  ut_assert(data->med->priority == data->med->provisioned_priority);
  ut_assert(data->high->priority == data->high->provisioned_priority);

  mutex_unlock(&data->mutex);

  ut_assert(data->low->state == STATE_READY);
  ut_assert(data->med->state == STATE_READY);
  ut_assert(data->low->priority == data->low->provisioned_priority);
  ut_assert(data->med->priority == data->med->provisioned_priority);
  ut_assert(data->high->priority == data->high->provisioned_priority);

  return NULL;
}

static __task void * task_test_mutex_priority1_med(void * arg)
{
  struct test_mutex_priority1_data * data = (struct test_mutex_priority1_data *)arg;

  ut_assert(data->low->state == STATE_READY);
  ut_assert(data->high->state == STATE_SLEEP);
  ut_assert(data->low->priority == data->low->provisioned_priority);
  ut_assert(data->med->priority == data->med->provisioned_priority);
  ut_assert(data->high->priority == data->high->provisioned_priority);

  task_delay(10);
  mutex_lock(&data->mutex);

  ut_assert(data->low->state == STATE_READY);
  ut_assert(data->high->state == STATE_ZOMBIE);
  ut_assert(data->low->priority == data->low->provisioned_priority);
  ut_assert(data->med->priority == data->med->provisioned_priority);
  ut_assert(data->high->priority == data->high->provisioned_priority);

  mutex_unlock(&data->mutex);

  ut_assert(data->low->state == STATE_READY);
  ut_assert(data->high->state == STATE_ZOMBIE);
  ut_assert(data->low->priority == data->low->provisioned_priority);
  ut_assert(data->med->priority == data->med->provisioned_priority);

  return NULL;
}

static __task void * task_test_mutex_priority1_low(void * arg)
{
  struct test_mutex_priority1_data * data = (struct test_mutex_priority1_data *)arg;

  ut_assert(data->med->state == STATE_SLEEP);
  ut_assert(data->high->state == STATE_SLEEP);
  ut_assert(data->low->priority == data->low->provisioned_priority);
  ut_assert(data->med->priority == data->med->provisioned_priority);
  ut_assert(data->high->priority == data->high->provisioned_priority);

  mutex_lock(&data->mutex);

  ut_assert(data->med->state == STATE_SLEEP);
  ut_assert(data->high->state == STATE_SLEEP);
  ut_assert(data->low->priority == data->low->provisioned_priority);
  ut_assert(data->med->priority == data->med->provisioned_priority);
  ut_assert(data->high->priority == data->high->provisioned_priority);

  task_delay(15);

  ut_assert(data->med->state == STATE_MUTEX);
  ut_assert(data->high->state == STATE_SLEEP);
  ut_assert(data->low->priority == data->med->provisioned_priority);
  ut_assert(data->med->priority == data->med->provisioned_priority);
  ut_assert(data->high->priority == data->high->provisioned_priority);

  task_delay(10);

  ut_assert(data->med->state == STATE_MUTEX);
  ut_assert(data->high->state == STATE_MUTEX);
  ut_assert(data->low->priority == data->high->provisioned_priority);
  ut_assert(data->med->priority == data->med->provisioned_priority);
  ut_assert(data->high->priority == data->high->provisioned_priority);

  mutex_unlock(&data->mutex);

  ut_assert(data->med->state == STATE_ZOMBIE);
  ut_assert(data->high->state == STATE_ZOMBIE);
  ut_assert(data->low->priority == data->low->provisioned_priority);

  return NULL;
}

static void test_mutex_priority1(void)
{
  struct test_mutex_priority1_data data = {
    .high = &tasks[0],
    .med = &tasks[1],
    .low = &tasks[2]
  };
  mutex_init(&data.mutex);
  task_init(&tasks[0], task_test_mutex_priority1_high, &data, stacks[0], STACK_SIZE, 6);
  task_init(&tasks[1], task_test_mutex_priority1_med, &data, stacks[1], STACK_SIZE, 5);
  task_init(&tasks[2], task_test_mutex_priority1_low, &data, stacks[2], STACK_SIZE, 4);
  // delay here so that the we don't influence any priorities by blocking
  task_delay(50);
  ut_assert(data.high->state == STATE_ZOMBIE);
  ut_assert(data.med->state == STATE_ZOMBIE);
  ut_assert(data.low->state == STATE_ZOMBIE);
  task_wait(&data.high);
  ut_assert(data.high->state == STATE_DEAD);
  task_wait(&data.med);
  ut_assert(data.med->state == STATE_DEAD);
  task_wait(&data.low);
  ut_assert(data.low->state == STATE_DEAD);
}

struct test_mutex_priority2_data
{
  struct task * high;
  struct task * low;
  struct mutex mutex;
};

static __task void * task_test_mutex_priority2_high(void * arg)
{
  struct test_mutex_priority2_data * data = (struct test_mutex_priority2_data *)arg;

  ut_assert(data->low->state == STATE_READY);
  ut_assert(data->low->priority == data->low->provisioned_priority);
  ut_assert(data->high->priority == data->high->provisioned_priority);

  mutex_lock(&data->mutex);

  ut_assert(data->low->state == STATE_READY);
  ut_assert(data->low->priority == data->low->provisioned_priority);
  ut_assert(data->high->priority == data->high->provisioned_priority);

  task_delay(10);

  ut_assert(data->low->state == STATE_MUTEX);
  ut_assert(data->low->priority == data->low->provisioned_priority);
  ut_assert(data->high->priority == data->high->provisioned_priority);

  mutex_unlock(&data->mutex);

  ut_assert(data->low->state == STATE_READY);
  ut_assert(data->low->priority == data->low->provisioned_priority);
  ut_assert(data->high->priority == data->high->provisioned_priority);

  return NULL;
}

static __task void * task_test_mutex_priority2_low(void * arg)
{
  struct test_mutex_priority2_data * data = (struct test_mutex_priority2_data *)arg;

  ut_assert(data->high->state == STATE_SLEEP);
  ut_assert(data->low->priority == data->low->provisioned_priority);
  ut_assert(data->high->priority == data->high->provisioned_priority);

  mutex_lock(&data->mutex);

  ut_assert(data->high->state == STATE_ZOMBIE);
  ut_assert(data->low->priority == data->low->provisioned_priority);
  ut_assert(data->high->priority == data->high->provisioned_priority);

  mutex_unlock(&data->mutex);

  ut_assert(data->high->state == STATE_ZOMBIE);
  ut_assert(data->low->priority == data->low->provisioned_priority);
  ut_assert(data->high->priority == data->high->provisioned_priority);

  return NULL;
}

static void test_mutex_priority2(void)
{
  struct test_mutex_priority2_data data = {
    .high = &tasks[0],
    .low = &tasks[1]
  };
  mutex_init(&data.mutex);
  task_init(&tasks[0], task_test_mutex_priority2_high, &data, stacks[0], STACK_SIZE, 6);
  task_init(&tasks[1], task_test_mutex_priority2_low, &data, stacks[1], STACK_SIZE, 5);
  // delay here so that the we don't influence any priorities by blocking
  task_delay(50);
  ut_assert(data.high->state == STATE_ZOMBIE);
  ut_assert(data.low->state == STATE_ZOMBIE);
  task_wait(&data.high);
  ut_assert(data.high->state == STATE_DEAD);
  task_wait(&data.low);
  ut_assert(data.low->state == STATE_DEAD);
}
