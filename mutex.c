/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <kevinmottashed@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * -Kevin Mottashed
 * ----------------------------------------------------------------------------
 */


#include "mutex.h"

#include "manticore.h"

#include "system.h"
#include "syscall.h"
#include "utils.h"
#include "kernel.h"
#include "heap.h"

#include <assert.h>

void mutex_init(struct mutex * mutex, uint32_t options)
{
  static uint8_t mutex_id_counter = 0;
  mutex->id = mutex_id_counter++;
  mutex->owner = NULL;
  mutex->locked = 0;
  mutex->recursive = options & MUTEX_ATTR_RECURSIVE;
  list_init(&mutex->waiting_tasks);
}

void mutex_lock(struct mutex * mutex)
{
  mutex_timed_lock(mutex, 0);
}

bool mutex_trylock(struct mutex * mutex)
{
  kernel_scheduler_disable();
  if (mutex->locked && (!mutex->recursive || mutex->owner != running_task))
  {
    kernel_scheduler_enable();
    return false;
  }
  else
  {
    assert(mutex->locked != UINT8_MAX);
    mutex->locked++;
    mutex->owner = running_task;
    kernel_scheduler_enable();
    return true;
  }
}

bool mutex_timed_lock(struct mutex * mutex, uint32_t milliseconds)
{
  kernel_scheduler_disable();
  if (!mutex->locked || (mutex->recursive && mutex->owner == running_task))
  {
    // The mutex is already ours or unlocked. Just take it.
    assert(mutex->locked != UINT8_MAX);
    mutex->locked++;
    mutex->owner = running_task;
    kernel_scheduler_enable();
    return true;
  }
  else
  {
    running_task->mutex = mutex;
    running_task->sleep = milliseconds;
    SVC_MUTEX_LOCK();
    return running_task->mutex_locked;
  }
}

void mutex_unlock(struct mutex * mutex)
{
  assert(mutex->locked);
  assert(mutex->owner == running_task);
  kernel_scheduler_disable();
  if (mutex->locked == 1 && !list_empty(&mutex->waiting_tasks))
  {
    // Another task is waiting for this mutex.
    // Let the kernel run to unblock it.
    running_task->mutex = mutex;
    SVC_MUTEX_UNLOCK();
  }
  else
  {
    // The current task still has the mutex
    // or no one else is waiting for it.
    mutex->locked--;
    if (!mutex->locked)
      mutex->owner = NULL;
    kernel_scheduler_enable();
  }
}
