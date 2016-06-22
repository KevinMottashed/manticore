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

#include "list.h"
#include "task.h"

#include <stdint.h>
#include <stdbool.h>

struct mutex
{
  uint8_t id; // Unique identifier for this mutex.

  // Number of times the mutex has been locked.
  // Can only be 0 or 1 for a non-recursive mutex.
  uint8_t locked;
  bool recursive;
  struct task * owner; // The task that owns this mutex.

  // The list of tasks waiting for this mutex.
  struct list_head waiting_tasks;
};

#endif
