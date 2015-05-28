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

#define SYSCALL_NONE          (0) // No syscall was made
#define SYSCALL_YIELD         (1) // Task wishes to yield to another
#define SYSCALL_SLEEP         (2) // Sleep for x milli seconds
#define SYSCALL_MUTEX_LOCK    (3) // Failed to lock a mutex
#define SYSCALL_MUTEX_UNLOCK  (4) // Unlock a mutex with tasks waiting on it
#define SYSCALL_CHANNEL_SEND  (5) // Send a message to a channel
#define SYSCALL_CHANNEL_RECV  (6) // Receive a message from a channel
#define SYSCALL_CHANNEL_REPLY (7) // Reply to a message on a channel

// Macros to do the system calls
#define SVC_YIELD()           asm ("SVC #1")
#define SVC_SLEEP()           asm ("SVC #2")
#define SVC_MUTEX_LOCK()      asm ("SVC #3")
#define SVC_MUTEX_UNLOCK()    asm ("SVC #4")
#define SVC_CHANNEL_SEND()    asm ("SVC #5")
#define SVC_CHANNEL_RECV()    asm ("SVC #6")
#define SVC_CHANNEL_REPLY()   asm ("SVC #7")

// The context that needs to be saved when send, receive or reply blocked.
typedef struct channel_context_s
{
  // The channel that we're block on
  struct channel_s * c;
  
  // Used for storing the send/recv buffer.
  void * msg;
  size_t len;
  
  // Used for storing the reply buffer.
  void * reply;
  size_t * replyLen;
} channel_context_t;
   
typedef union syscall_context_u
{
  unsigned int sleep;
  struct mutex_s * mutex;
  channel_context_t channel;
} syscall_context_t;

#endif
