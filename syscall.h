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

#include "mutex.h"
#include "channel.h"

#define SYSCALL_NONE         (0) // No syscall was made
#define SYSCALL_YIELD        (1) // Task wishes to yield to another
#define SYSCALL_SLEEP        (2) // Sleep for x milli seconds
#define SYSCALL_MUTEX_LOCK   (3) // Failed to lock a mutex
#define SYSCALL_MUTEX_UNLOCK (4) // Unlock a mutex with tasks waiting on it
#define SYSCALL_CHANNEL_SEND (5) // Send a message to a channel
#define SYSCALL_CHANNEL_RECV (6) // Receive a message from a channel
#define SYSCALL_CHANNEL_RPLY (7) // Reply to a message on a channel

typedef union syscall_context_u
{
  unsigned int sleep;
  mutex_t * mutex;
  channel_context_t channel;
} syscall_context_t;

extern syscall_context_t syscallContext;

void yield(void);

void sleep(unsigned int seconds);
void delay(unsigned int ms);

#endif
