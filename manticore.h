/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <kevinmottashed@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * -Kevin Mottashed
 * ----------------------------------------------------------------------------
 */

#ifndef MANTICORE_H
#define MANTICORE_H

#include "task.h"
#include "mutex.h"
#include "channel.h"

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

// --------------------------------------
// Kernel
// --------------------------------------

/**
 * Initialize the kernel.
 * This must be called before creating any tasks or sychronization primitives.
 */
void manticore_init(void);

/**
 * Start running the kernel.
 * @return This function never returns.
 */
void manticore_main(void);


// --------------------------------------
// Task
// --------------------------------------

// Entry signature for all tasks
// The __task specifier informs the compiler that the function is an
// RTOS task. This means that the normal calling conventions don't need
// to be followed and that it only needs to preserve the LR register.
typedef void * (__task * task_entry_t)(void *);

struct task;

/**
 * Initialize a new task.
 * @param task The task to initialize.
 * @param entry The entry point for the new task.
 * @param arg The argument passed to the new task.
 * @param stack The memory used for the stack.
 * @param stackSize The size of the stack.
 * @param priority The priority of the new task.
 */
void task_init(struct task * task,
               task_entry_t entry,
               void * arg,
               void * stack,
               uint32_t stackSize,
               uint8_t priority);

/**
 * Wait for a task to return.
 * If <task> is NULL then wait for any child.
 * If <task> points to NULL then wait for any child and update <task> with the child that terminated.
 * If <task> points to non-NULL then wait for that child.
 * @param task The task to wait for.
 * @return The value that the task returned.
 */
void * task_wait(struct task ** task);

/**
 * Get a tasks priority.
 * @param task The task whose priority will be returned. Passing NULL will
 *             return the callers priority.
 * @return The priority of <task>.
 */
uint8_t task_get_priority(struct task * task);

/**
 * Put the calling task to sleep.
 * @param seconds The number of seconds to sleep.
 */
void task_sleep(unsigned int seconds);

/**
 * Put the calling task to sleep.
 * @param milliseconds The number of milliseconds to sleep.
 */
void task_delay(unsigned int milliseconds);

/**
 * Yield to the next task.
 * This will cause the calling task to give up its remaining time slice.
 */
void task_yield(void);

// --------------------------------------
// Mutex
// --------------------------------------

struct mutex;

// Controls if the mutex can be locked recursively.
#define MUTEX_ATTR_NON_RECURSIVE                (0 << 0)
#define MUTEX_ATTR_RECURSIVE                    (1 << 0)

// The default mutex is non-recursive.
#define MUTEX_ATTR_DEFAULT                      MUTEX_ATTR_NON_RECURSIVE

/**
 * Initialize a new mutex.
 * @param mutex The mutex to initialize.
 * @param attributes The attributes to initialize the mutex with. See MUTEX_ATTR_*.
 */
void mutex_init(struct mutex * mutex, uint32_t attributes);

/**
 * Lock a mutex.
 * This call will block if the mutex is already locked.
 * @param mutex The mutex to lock.
 */
void mutex_lock(struct mutex * mutex);

/**
 * Try to lock a mutex.
 * This call won't block if the mutex is already locked.
 * @param mutex The mutex to lock.
 * @return True if the mutex was locked.
 */
bool mutex_trylock(struct mutex * mutex);

/**
 * Try to lock a mutex. Give up after the elapsed time.
 * @param mutex The mutex to lock
 * @param milliseconds The amount of time to wait before giving up
 * @return True if the mutex was locked.
 */
bool mutex_timed_lock(struct mutex * mutex, uint32_t milliseconds);

/**
 * Unlock a mutex.
 * @param mutex The mutex to unlock.
 */
void mutex_unlock(struct mutex * mutex);

// --------------------------------------
// Channel
// --------------------------------------

struct channel;

/**
 * Initialize a new channel.
 * @param channel The channel to initialize.
 */
void channel_init(struct channel * channel);

/**
 * Send a message through a channel.
 * @param channel The channel to send the message to.
 * @param data The data to send.
 * @param len The length of data to send.
 * @param reply The buffer where the reply can be stored.
 * @param reply_len The length of the reply buffer.
 */
void channel_send(struct channel * channel,
                  void * data,
                  size_t len,
                  void * reply,
                  size_t * reply_len);

/**
 * Receive a message from a channel.
 * @param channel The channel to receive a message from.
 * @param data The buffer where the message can be stored.
 * @param len The length of the receive buffer.
 * @return The size of the received message.
 */
size_t channel_recv(struct channel * channel, void * data, size_t len);

/**
 * Reply to a previously received message.
 * @param channel The channel to reply to.
 * @param data The data to reply with.
 * @param len The length of the reply.
 */
void channel_reply(struct channel * channel, void * data, size_t len);

#endif
