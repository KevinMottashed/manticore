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

mutex_handle_t mutex_create(void)
{
  static uint8_t mutexIdCounter = 0;

  mutex_t * mutex = heap_malloc(sizeof(*mutex));
  assert(mutex != NULL);

  mutex->id = mutexIdCounter++;
  mutex->owner = NULL;
  mutex->locked = false;
  list_init(&mutex->waiting_tasks);
  return mutex;
}

void mutex_lock(mutex_t * mutex)
{
  kernel_scheduler_disable();

  if (!mutex->locked)
  {
    // The mutex is unlocked. Just take it.
    mutex->locked = true;
    mutex->owner = runningTask;
    kernel_scheduler_enable();
  }
  else
  {
    // The mutex is locked. Let the OS schedule the next task.
    runningTask->mutex = mutex;
    SVC_MUTEX_LOCK();
  }
}

bool mutex_trylock(mutex_t * mutex)
{
  kernel_scheduler_disable();
  if (mutex->locked)
  {
    kernel_scheduler_enable();
    return false;
  }
  else
  {
    mutex->locked = true;
    mutex->owner = runningTask;
    kernel_scheduler_enable();
    return true;
  }
}

void mutex_unlock(mutex_t * mutex)
{
  kernel_scheduler_disable();
  if (!list_empty(&mutex->waiting_tasks))
  {
    // Another task is waiting for this mutex.
    // Let the kernel run to unblock it.
    runningTask->mutex = mutex;
    SVC_MUTEX_UNLOCK();
  }
  else
  {
    // No one is waiting for this mutex.
    mutex->locked = false;
    mutex->owner = NULL;
    kernel_scheduler_enable();
  }
}
