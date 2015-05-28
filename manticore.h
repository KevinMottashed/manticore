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

// Handle for tasks.
typedef struct task_s * task_handle_t;

// Entry signature for all tasks
// The __task specifier informs the compiler that the function is an
// RTOS task. This means that the normal calling don't need to be followed
// and that it only needs to preserve the LR register.
typedef void * (__task * task_entry_t)(void *);

/**
 * Create a new task.
 * @param entry The entry point for the new task.
 * @param arg The argument passed to the new task.
 * @param stack The memory used for the stack.
 * @param stackSize The size of the stack.
 * @param priority The priority of the new task.
 * @return A handle to the newly created task.
 */
task_handle_t task_create(task_entry_t entry, 
                          void * arg, 
                          void * stack, 
                          uint32_t stackSize, 
                          uint8_t priority);

/**
 * Get a tasks priority.
 * @param task The task whose priority will be returned. Passing NULL will
 *             return the callers priority.
 * @return The priority of <task>.
 */
uint8_t task_get_priority(task_handle_t task);

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

// Handle for mutexes.
typedef struct mutex_s * mutex_handle_t;

/**
 * Create a new mutex.
 * @return A handle to the newly created mutex.
 */
mutex_handle_t mutex_create(void);

/**
 * Lock a mutex.
 * This call will block if the mutex is already locked.
 * @param mutex The mutex to lock.
 */
void mutex_lock(mutex_handle_t mutex);

/**
 * Try to lock a mutex.
 * This call won't block if the mutex is already locked.
 * @param mutex The mutex to lock.
 * @return True if the mutex was lock.
 */
bool mutex_trylock(mutex_handle_t mutex);

/**
 * Unlock a mutex.
 * @param mutex The mutex to unlock.
 */
void mutex_unlock(mutex_handle_t mutex);

// --------------------------------------
// Channel
// --------------------------------------

// Handle for channels.
typedef struct channel_s * channel_handle_t;

/**
 * Create a new channel.
 * @return A handle to the newly created channel.
 */
channel_handle_t channel_create(void);

/**
 * Send a message through a channel.
 * @param channel The channel to send the message to.
 * @param data The data to send.
 * @param len The len of data to send.
 * @param reply The buffer where the reply can be stored.
 * @param replyLen The length of the reply buffer.
 */
void channel_send(channel_handle_t channel,
                  void * data,
                  size_t len,
                  void * reply,
                  size_t * replyLen);

/**
 * Receive a message from a channel.
 * @param channel The channel to receive a message from.
 * @param data The buffer where the message can be stored.
 * @param len The length of the receive buffer.
 */
void channel_recv(channel_handle_t channel, void * data, size_t len);

/**
 * Reply to a previously received message.
 * @param channel The channel to reply to.
 * @param data The data to reply with.
 * @param len The length of the reply.
 */
void channel_reply(channel_handle_t channel, void * data, size_t len);

#endif
