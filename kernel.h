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

#include "pqueue.h"
#include "task.h"

#include <stdint.h>

// The __task specifier informs the compiler that the function is an
// RTOS task. This means that the normal calling don't need to be followed
// and that it only needs to preserve the LR register.
typedef void (__task *task)(void *);

void kernel_init(void);
void kernel_main(void);

void kernel_create_task(task t, void * arg, void * stack, uint32_t stackSize, uint8_t priority);

extern pqueue_t readyQueue;
extern task_t * runningTask;

#endif
