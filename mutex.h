/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <kevinmottashed@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return. 
 * -Kevin Mottashed
 * ----------------------------------------------------------------------------
 */


#ifndef MUTEX_H
#define MUTEX_H

#include "pqueue.h"
#include "task.h"
   
#include <stdint.h>
#include <stdbool.h>

typedef struct mutex_s
{
  uint8_t id; // Unique identifier for this mutex.
  bool locked;
  struct task_s * owner; // The task that owns this mutex.
  pqueue_t queue; // The queue of blocked tasks waiting for this mutex.
} mutex_t;

void mutex_init(mutex_t * mutex);

void mutex_lock(mutex_t * mutex);
bool mutex_trylock(mutex_t * mutex);

void mutex_unlock(mutex_t * mutex);

#endif
