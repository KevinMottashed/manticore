/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <kevinmottashed@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * -Kevin Mottashed
 * ----------------------------------------------------------------------------
 */

#ifndef KERNEL_H
#define KERNEL_H

#include "task.h"
#include "syscall.h"

#include <stdint.h>

// These can be used as a critical section to prevent context switching.
// One of the following must be done after disabling the scheduler:
// 1. Call kernel_scheduler_enable().
// 2. Do a system call.
void kernel_scheduler_disable(void);
void kernel_scheduler_enable(void);

// The list of all ready tasks
extern struct list_head ready_tasks;

// The task that's currently running.
extern struct task * running_task;

// True once manticore_main() has been called.
extern bool kernel_running;

#endif
