/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <kevinmottashed@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * -Kevin Mottashed
 * ----------------------------------------------------------------------------
 */

#ifndef SYSCALL_H
#define SYSCALL_H

#include <string.h>

#define SYSCALL_NONE          (0) // No system call was executed
#define SYSCALL_YIELD         (1) // Task wishes to yield to another
#define SYSCALL_SLEEP         (2) // Sleep for x milli seconds
#define SYSCALL_MUTEX_LOCK    (3) // Failed to lock a mutex
#define SYSCALL_MUTEX_UNLOCK  (4) // Unlock a mutex with tasks waiting on it
#define SYSCALL_CHANNEL_SEND  (5) // Send a message to a channel
#define SYSCALL_CHANNEL_RECV  (6) // Receive a message from a channel
#define SYSCALL_CHANNEL_REPLY (7) // Reply to a message on a channel
#define SYSCALL_TASK_RETURN   (8) // A task makes this syscall when it returns
#define SYSCALL_TASK_WAIT     (9) // Wait for a task to finish

// Macros to do the system calls
#define SVC_YIELD()           asm ("SVC #1")
#define SVC_SLEEP()           asm ("SVC #2")
#define SVC_MUTEX_LOCK()      asm ("SVC #3")
#define SVC_MUTEX_UNLOCK()    asm ("SVC #4")
#define SVC_CHANNEL_SEND()    asm ("SVC #5")
#define SVC_CHANNEL_RECV()    asm ("SVC #6")
#define SVC_CHANNEL_REPLY()   asm ("SVC #7")
#define SVC_TASK_RETURN()     asm ("SVC #8")
#define SVC_TASK_WAIT()       asm ("SVC #9")

#endif
