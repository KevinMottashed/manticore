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
#include "clock.h"

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

// TODO Break up these tests into a bunch of files.

extern __task void * Task_Busy_Yield(void * arg);
static void test_context_switching(void);

static __task void * task_test_get_priority(void * arg);
static void test_get_priority(void);

static __task void * task_test_sleep(void * arg);
static void test_sleep(void);

static __task void * task_test_delay(void * arg);
static void test_delay(void);

static __task void * task_test_yield(void * arg);
static void test_yield(void);

// Tests for mutexes
static __task void * task_test_mutex_lock(void * arg);
static void test_mutex_lock(void);

static __task void * task_test_mutex_trylock(void * arg);
static void test_mutex_trylock(void);

static __task void * task_test_mutex_priority1_high(void * arg);
static __task void * task_test_mutex_priority1_med(void * arg);
static __task void * task_test_mutex_priorityl_low(void * arg);
static void test_mutex_priority1(void);

static __task void * task_test_mutex_priority2_high(void * arg);
static __task void * task_test_mutex_priority2_low(void * arg);
static void test_mutex_priority2(void);

static __task void * task_test_mutex_timed_lock1(void * arg);
static __task void * task_test_mutex_timed_lock2(void * arg);
static void test_mutex_timed_lock(void);

// Tests for recursive mutexes.
static __task void * task_test_recursive_mutex_lock(void * arg);
static void test_recursive_mutex_lock(void);

static __task void * task_test_recursive_mutex_trylock(void * arg);
static void test_recursive_mutex_trylock(void);

static __task void * task_test_recursive_mutex_priority_high(void * arg);
static __task void * task_test_recursive_mutex_priority_low(void * arg);
static void test_recursive_mutex_priority(void);

// Tests for time slices
static __task void * task_test_sched_time_slice_length(void * arg);
static void test_sched_time_slice_length(void);

static __task void * task_test_sched_time_slice_yield1(void * arg);
static void test_sched_time_slice_yield1(void);

static __task void * task_test_sched_time_slice_yield2(void * arg);
static void test_sched_time_slice_yield2(void);

static __task void * task_test_sched_time_slice_sleep1(void * arg);
static void test_sched_time_slice_sleep1(void);

static __task void * task_test_sched_time_slice_sleep2(void * arg);
static void test_sched_time_slice_sleep2(void);

static __task void * task_test_sched_time_slice_mutex(void * arg);
static void test_sched_time_slice_mutex(void);

static __task void * task_test_sched_context_switch_performance(void * arg);
static void test_sched_context_switch_performance1(void);
static void test_sched_context_switch_performance2(void);

// Helper asserts
static void assert_full_time_slice(void);
static void assert_max_time_slice(void);

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
  test_mutex_timed_lock();
  test_recursive_mutex_lock();
  test_recursive_mutex_trylock();
  test_recursive_mutex_priority();
  test_sched_time_slice_length();
  test_sched_time_slice_yield1();
  test_sched_time_slice_yield2();
  test_sched_time_slice_sleep1();
  test_sched_time_slice_sleep2();
  test_sched_time_slice_mutex();
  test_sched_context_switch_performance1();
  test_sched_context_switch_performance2();
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
  mutex_init(&data.mutex, MUTEX_ATTR_DEFAULT);
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
  mutex_init(&data.mutex, MUTEX_ATTR_DEFAULT);
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
  mutex_init(&data.mutex, MUTEX_ATTR_DEFAULT);
  task_init(&tasks[0], task_test_mutex_priority1_high, &data, stacks[0], STACK_SIZE, 6);
  task_init(&tasks[1], task_test_mutex_priority1_med, &data, stacks[1], STACK_SIZE, 5);
  task_init(&tasks[2], task_test_mutex_priority1_low, &data, stacks[2], STACK_SIZE, 4);
  // Delay so that we don't influence any priorities by blocking.
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
  mutex_init(&data.mutex, MUTEX_ATTR_DEFAULT);
  task_init(&tasks[0], task_test_mutex_priority2_high, &data, stacks[0], STACK_SIZE, 6);
  task_init(&tasks[1], task_test_mutex_priority2_low, &data, stacks[1], STACK_SIZE, 5);
  // Delay so that we don't influence any priorities by blocking.
  task_delay(50);
  ut_assert(data.high->state == STATE_ZOMBIE);
  ut_assert(data.low->state == STATE_ZOMBIE);
  task_wait(&data.high);
  ut_assert(data.high->state == STATE_DEAD);
  task_wait(&data.low);
  ut_assert(data.low->state == STATE_DEAD);
}

struct test_mutex_timed_lock_data
{
  struct task * t1;
  struct task * t2;
  struct mutex mutex;
};

static __task void * task_test_mutex_timed_lock1(void * arg)
{
  struct test_mutex_timed_lock_data * data = (struct test_mutex_timed_lock_data*)arg;
  ut_assert(data->t2->state == STATE_READY);
  mutex_lock(&data->mutex);
  ut_assert(data->t2->state == STATE_READY);
  task_delay(10);
  ut_assert(data->t2->state == STATE_MUTEX);
  task_delay(10);
  ut_assert(data->t2->state == STATE_MUTEX);
  mutex_unlock(&data->mutex);
  ut_assert(data->t2->state == STATE_READY);
  return NULL;
}

static __task void * task_test_mutex_timed_lock2(void * arg)
{
  struct test_mutex_timed_lock_data * data = (struct test_mutex_timed_lock_data*)arg;
  ut_assert(data->t1->state == STATE_SLEEP);
  task_delay(5);
  bool locked = mutex_timed_lock(&data->mutex, 10);
  ut_assert(!locked);
  ut_assert(data->t1->state == STATE_SLEEP);
  locked = mutex_timed_lock(&data->mutex, 10);
  ut_assert(locked);
  ut_assert(data->t1->state == STATE_ZOMBIE);
  mutex_unlock(&data->mutex);
  return NULL;
}

static void test_mutex_timed_lock(void)
{
  struct test_mutex_timed_lock_data data = {
    .t1 = &tasks[0],
    .t2 = &tasks[1],
  };
  mutex_init(&data.mutex, MUTEX_ATTR_DEFAULT);
  task_init(&tasks[0], task_test_mutex_timed_lock1, &data, stacks[0], STACK_SIZE, 5);
  task_init(&tasks[1], task_test_mutex_timed_lock2, &data, stacks[1], STACK_SIZE, 5);
  // Delay so that we don't influence any priorities by blocking.
  task_delay(50);
  ut_assert(data.t1->state == STATE_ZOMBIE);
  ut_assert(data.t2->state == STATE_ZOMBIE);
  task_wait(&data.t1);
  ut_assert(data.t1->state == STATE_DEAD);
  task_wait(&data.t2);
  ut_assert(data.t2->state == STATE_DEAD);
}

static __task void * task_test_recursive_mutex_lock(void * arg)
{
  // Make sure we can lock/unlock a bunch of times.
  struct mutex * mutex = (struct mutex*)arg;
  for (int32_t i = 0; i < 10; ++i)
    mutex_lock(mutex);
  for (int32_t i = 0; i < 10; ++i)
    mutex_unlock(mutex);
  return NULL;
}

static void test_recursive_mutex_lock(void)
{
  struct mutex mutex;
  mutex_init(&mutex, MUTEX_ATTR_RECURSIVE);
  task_init(&tasks[0], task_test_recursive_mutex_lock, &mutex, stacks[0], STACK_SIZE, 5);
  task_wait(NULL);
}

static __task void * task_test_recursive_mutex_trylock(void * arg)
{
  // Make sure we can trylock after locking.
  struct mutex * mutex = (struct mutex*)arg;
  mutex_lock(mutex);
  bool locked = mutex_trylock(mutex);
  ut_assert(locked);
  mutex_unlock(mutex);
  mutex_unlock(mutex);
  return NULL;
}

static void test_recursive_mutex_trylock(void)
{
  struct mutex mutex;
  mutex_init(&mutex, MUTEX_ATTR_RECURSIVE);
  task_init(&tasks[0], task_test_recursive_mutex_trylock, &mutex, stacks[0], STACK_SIZE, 5);
  task_wait(NULL);
}

static __task void * task_test_recursive_mutex_priority_high(void * arg)
{
  struct mutex * mutex = (struct mutex*)arg;
  struct task * high = &tasks[0];
  struct task * low = &tasks[1];

  ut_assert(low->state == STATE_READY);
  ut_assert(low->priority == low->provisioned_priority);
  ut_assert(high->priority == high->provisioned_priority);

  mutex_lock(mutex);

  ut_assert(low->state == STATE_READY);
  ut_assert(low->priority == low->provisioned_priority);
  ut_assert(high->priority == high->provisioned_priority);

  // Let the other task run.
  task_delay(5);

  // Make sure the state of the other task doesn't change when
  // we lock the mutex a bunch of times.
  for (int32_t i = 0; i < 3; ++i) {
    ut_assert(low->state == STATE_MUTEX);
    ut_assert(low->priority == low->provisioned_priority);
    ut_assert(high->priority == high->provisioned_priority);
    mutex_lock(mutex);
  }

  // Unlock the mutex to let other task have it.
  for (int32_t i = 0; i < 4; ++i) {
    ut_assert(low->state == STATE_MUTEX);
    ut_assert(low->priority == low->provisioned_priority);
    ut_assert(high->priority == high->provisioned_priority);
    mutex_unlock(mutex);
  }

  // The other task should now have the mutex.
  ut_assert(low->state == STATE_READY);
  ut_assert(low->priority == low->provisioned_priority);
  ut_assert(high->priority == high->provisioned_priority);

  task_delay(5);

  // Lock/unlock it.
  mutex_lock(mutex);
  ut_assert(low->state == STATE_READY);
  ut_assert(low->priority == low->provisioned_priority);
  ut_assert(high->priority == high->provisioned_priority);
  mutex_unlock(mutex);
  ut_assert(low->state == STATE_READY);
  ut_assert(low->priority == low->provisioned_priority);
  ut_assert(high->priority == high->provisioned_priority);

  return NULL;
}

static __task void * task_test_recursive_mutex_priority_low(void * arg)
{
  struct mutex * mutex = (struct mutex*)arg;
  struct task * high = &tasks[0];
  struct task * low = &tasks[1];

  ut_assert(high->state == STATE_SLEEP);
  ut_assert(low->priority == low->provisioned_priority);
  ut_assert(high->priority == high->provisioned_priority);

  mutex_lock(mutex);

  ut_assert(high->state == STATE_SLEEP);
  ut_assert(low->priority == low->provisioned_priority);
  ut_assert(high->priority == high->provisioned_priority);

  // Make sure we can lock it a bunch of times.
  for (int32_t i = 0; i < 3; ++i) {
    ut_assert(high->state == STATE_SLEEP);
    ut_assert(low->priority == low->provisioned_priority);
    ut_assert(high->priority == high->provisioned_priority);
    mutex_lock(mutex);
  }

  // Wait for the other task to try to take back the mutex.
  // We should inherit their priority.
  task_delay(10);
  ut_assert(high->state == STATE_MUTEX);
  ut_assert(low->priority == high->provisioned_priority);
  ut_assert(high->priority == high->provisioned_priority);

  // Make sure we can still lock it a bunch of times.
  for (int32_t i = 0; i < 3; ++i) {
    ut_assert(high->state == STATE_MUTEX);
    ut_assert(low->priority == high->provisioned_priority);
    ut_assert(high->priority == high->provisioned_priority);
    mutex_lock(mutex);
  }

  // Unlock it to let the other task take it.
  for (int32_t i = 0; i < 7; ++i) {
    ut_assert(high->state == STATE_MUTEX);
    ut_assert(low->priority == high->provisioned_priority);
    ut_assert(high->priority == high->provisioned_priority);
    mutex_unlock(mutex);
  }

  // The other task is done and we've lost our high priority.
  ut_assert(high->state == STATE_ZOMBIE);
  ut_assert(low->priority == low->provisioned_priority);
  ut_assert(high->priority == high->provisioned_priority);

  return NULL;
}

static void test_recursive_mutex_priority(void)
{
  struct mutex mutex;
  mutex_init(&mutex, MUTEX_ATTR_RECURSIVE);
  task_init(&tasks[0], task_test_recursive_mutex_priority_high, &mutex, stacks[0], STACK_SIZE, 10);
  task_init(&tasks[1], task_test_recursive_mutex_priority_low, &mutex, stacks[1], STACK_SIZE, 5);
  task_delay(20);
  struct task * finished = NULL;
  task_wait(&finished);
  ut_assert(finished == &tasks[0]);
  finished = NULL;
  task_wait(&finished);
  ut_assert(finished == &tasks[1]);
}

static __task void * task_test_sched_time_slice_length(void * arg)
{
  // Make sure we got a full time slice.
  volatile uint32_t * done = (volatile uint32_t *)arg;
  assert_full_time_slice();
  ++(*done);
  while (*done != 2);
  return NULL;
}

static void test_sched_time_slice_length(void)
{
  uint32_t done = 0;
  task_init(&tasks[0], task_test_sched_time_slice_length, &done, stacks[0], STACK_SIZE, 5);
  task_init(&tasks[1], task_test_sched_time_slice_length, &done, stacks[1], STACK_SIZE, 5);
  // Delay so that we don't influence any priorities by blocking.
  task_delay(50);
  ut_assert(tasks[0].state == STATE_ZOMBIE);
  ut_assert(tasks[1].state == STATE_ZOMBIE);
  task_wait(NULL);
  task_wait(NULL);
  ut_assert(tasks[0].state == STATE_DEAD);
  ut_assert(tasks[1].state == STATE_DEAD);
  return;
}

static __task void * task_test_sched_time_slice_yield1(void * arg)
{
  // Wait until we have 1ms left in our time slice and yield.
  // We should get a new time slice after yielding.
  volatile uint32_t * done = (volatile uint32_t *)arg;
  assert_full_time_slice();
  while (SysTick->VAL > SYSTICK_HZ / 1000);
  task_yield();
  assert_full_time_slice();
  ++(*done);
  while (*done != 2);
  return NULL;
}

static void test_sched_time_slice_yield1(void)
{
  uint32_t done = 0;
  task_init(&tasks[0], task_test_sched_time_slice_yield1, &done, stacks[0], STACK_SIZE, 5);
  task_init(&tasks[1], task_test_sched_time_slice_yield1, &done, stacks[1], STACK_SIZE, 5);
  // Delay so that we don't influence any priorities by blocking.
  task_delay(100);
  ut_assert(tasks[0].state == STATE_ZOMBIE);
  ut_assert(tasks[1].state == STATE_ZOMBIE);
  task_wait(NULL);
  task_wait(NULL);
  ut_assert(tasks[0].state == STATE_DEAD);
  ut_assert(tasks[1].state == STATE_DEAD);
  return;
}

static __task void * task_test_sched_time_slice_yield2(void * arg)
{
  // This task runs alone so it should always get the maximum time slice.
  // Wait until we've used 10ms of our time slice and yield.
  // We should get a new maximum time slice after yielding.
  assert_max_time_slice();
  while (SysTick->VAL > 0xffffff - SYSTICK_HZ / 100);
  task_yield();
  assert_max_time_slice();
  return NULL;
}

static void test_sched_time_slice_yield2(void)
{
  task_init(&tasks[0], task_test_sched_time_slice_yield2, NULL, stacks[0], STACK_SIZE, 5);
  task_wait(NULL);
  ut_assert(tasks[0].state == STATE_DEAD);
  return;
}

static __task void * task_test_sched_time_slice_sleep1(void * arg)
{
  // Wait until we have 1ms left in our time slice and sleep.
  // We should get a new time slice after sleeping.
  volatile uint32_t * done = (volatile uint32_t *)arg;
  assert_full_time_slice();
  while (SysTick->VAL > SYSTICK_HZ / 1000);
  task_delay(5);
  assert_full_time_slice();
  ++(*done);
  while (*done != 2);
  return NULL;
}

static void test_sched_time_slice_sleep1(void)
{
  uint32_t done = 0;
  task_init(&tasks[0], task_test_sched_time_slice_sleep1, &done, stacks[0], STACK_SIZE, 5);
  task_init(&tasks[1], task_test_sched_time_slice_sleep1, &done, stacks[1], STACK_SIZE, 5);
  // Delay so that we don't influence any priorities by blocking.
  task_delay(100);
  ut_assert(tasks[0].state == STATE_ZOMBIE);
  ut_assert(tasks[1].state == STATE_ZOMBIE);
  task_wait(NULL);
  task_wait(NULL);
  ut_assert(tasks[0].state == STATE_DEAD);
  ut_assert(tasks[1].state == STATE_DEAD);
  return;
}

static __task void * task_test_sched_time_slice_sleep2(void * arg)
{
  // This task runs alone so it should always get the maximum time slice.
  // Wait until we've used 10ms of our time slice and sleep.
  // We should get a new maximum time slice after sleeping.
  assert_max_time_slice();
  while (SysTick->VAL > 0xffffff - SYSTICK_HZ / 100);
  task_delay(5);
  assert_max_time_slice();
  return NULL;
}

static void test_sched_time_slice_sleep2(void)
{
  task_init(&tasks[0], task_test_sched_time_slice_sleep2, NULL, stacks[0], STACK_SIZE, 5);
  task_wait(NULL);
  ut_assert(tasks[0].state == STATE_DEAD);
  return;
}

static __task void * task_test_sched_time_slice_mutex1(void * arg)
{
  struct mutex * mutex = (struct mutex *)arg;
  assert_full_time_slice();
  mutex_lock(mutex);
  task_delay(5);
  assert_max_time_slice();
  while (SysTick->VAL > SYSTICK_HZ / 200); // Waste half the time slice
  mutex_unlock(mutex);
  ut_assert(SysTick->LOAD <= SYSTICK_HZ / 200); // Make sure we got a half time slice or less.
  task_delay(5);
  return NULL;
}

static __task void * task_test_sched_time_slice_mutex2(void * arg)
{
  struct mutex * mutex = (struct mutex *)arg;
  assert_full_time_slice();
  task_delay(2);
  assert_full_time_slice();
  mutex_lock(mutex);
  assert_full_time_slice();
  mutex_unlock(mutex);
  return NULL;
}

static void test_sched_time_slice_mutex(void)
{
  struct mutex mutex;
  mutex_init(&mutex, MUTEX_ATTR_NON_RECURSIVE);
  task_init(&tasks[0], task_test_sched_time_slice_mutex1, &mutex, stacks[0], STACK_SIZE, 5);
  task_init(&tasks[1], task_test_sched_time_slice_mutex2, &mutex, stacks[1], STACK_SIZE, 5);

  // The 2nd task should finish first.
  struct task * finished = NULL;
  task_wait(&finished);
  ut_assert(finished == &tasks[1]);
  ut_assert(tasks[0].state == STATE_SLEEP);
  ut_assert(tasks[1].state == STATE_DEAD);

  // Wait for the first task to finish.
  finished = NULL;
  task_wait(&finished);
  ut_assert(finished == &tasks[0]);
  ut_assert(tasks[0].state == STATE_DEAD);
  return;
}

static __task void * task_test_sched_context_switch_performance(void * arg)
{
  volatile bool * stop = (volatile bool*)arg;
  uint32_t count = 0;
  while (!*stop) {
    ++count;
    task_yield();
  }
  return (void*)count;
}

static void test_sched_context_switch_performance1(void)
{
  // A performance test to see how fast we can context switch between
  // NUM_TASKS equal priority tasks.
  bool stop = false;
  for (uint32_t i = 0; i < NUM_TASKS; ++i) {
    task_init(&tasks[i], task_test_sched_context_switch_performance, &stop, stacks[i], STACK_SIZE, 5);
  }
  task_sleep(1);
  stop = true;

  uint32_t switches = 0;
  for (uint32_t i = 0; i < NUM_TASKS; ++i) {
    switches += (uint32_t)task_wait(NULL);
  }

  // Make sure the performance is in the 99%-110% range of when it was last measured.
  // In release mode it was last measured at 49458 switches per second (20us/switch).
#ifdef NDEBUG
  ut_assert(switches >= 49458 * 99 / 100 && switches <= 49458 * 110 / 100);
#else
  ut_assert(switches >= 31496 * 99 / 100 && switches <= 31496 * 110 / 100);
#endif
}

static void test_sched_context_switch_performance2(void)
{
  // A performance test to see how fast we can context switch between
  // NUM_TASKS / 2 equal priority tasks and NUM_TASKS / 2 lower priority tasks.
  bool stop = false;
  for (uint32_t i = 0; i < NUM_TASKS; ++i) {
    uint8_t priority = i < NUM_TASKS / 2 ? 5 : 4;
    task_init(&tasks[i], task_test_sched_context_switch_performance, &stop, stacks[i], STACK_SIZE, priority);
  }
  task_sleep(1);
  stop = true;

  uint32_t switches = 0;
  for (uint32_t i = 0; i < NUM_TASKS; ++i) {
    switches += (uint32_t)task_wait(NULL);
  }

  // Make sure the performance is in the 99%-110% range of when it was last measured.
  // In release mode it was last measured at 41504 switches per second (24us/switch).
#ifdef NDEBUG
  ut_assert(switches >= 41504 * 99 / 100 && switches <= 41504 * 110 / 100);
#else
  ut_assert(switches >= 26667 * 99 / 100 && switches <= 26667 * 110 / 100);
#endif
}

static void assert_full_time_slice(void)
{
  // Make sure that we were given a 10ms time slice
  // and that there's at least 9990us left.
  ut_assert(SysTick->LOAD == SYSTICK_HZ / 100);
  ut_assert(SysTick->VAL >= SYSTICK_HZ / 100 - SYSTICK_HZ / 100000);
}

static void assert_max_time_slice(void)
{
  // Make sure that we were given the maximum time slice
  // and that there's at most 10us used.
  ut_assert(SysTick->LOAD == 0xffffff);
  ut_assert(SysTick->VAL >= 0xffffff - SYSTICK_HZ / 100000);
}
