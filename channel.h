/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <kevinmottashed@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return. 
 * -Kevin Mottashed
 * ----------------------------------------------------------------------------
 */

/*
 * A channel is a form of synchronous message passing.
 * The sending task blocks on channel_send() until
 * the receiving task calls channel_recv() and channel_reply().
 * The sending task will unblock when it has received a reply.
 */

#ifndef CHANNEL_H
#define CHANNEL_H

#include "pqueue.h"
#include "task.h"

typedef struct channel_s
{
  int id;
  pqueue_t sendQueue; // The queue of tasks that have sent messages.
  struct task_s * receive; // The task waiting for a message.
  struct task_s * reply; // The task waiting for a reply.
  struct task_s * server; // The task currently handing a message.
} channel_t;

void channel_init(channel_t * c);

// Send a message to a channel.
// This call will block until another task replies to the message.
void channel_send(channel_t * c,
                  void * data,
                  size_t len,
                  void * reply,
                  size_t * replyLen);

// Receive a message from a channel.
// This call will block if no task has sent a message to us.
void channel_recv(channel_t * c, void * data, size_t len);

// Reply to a message.
void channel_reply(channel_t * c, void * data, size_t len);

#endif
