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

#include "list.h"
#include "task.h"

typedef struct channel_s
{
  int id;

  // The list of tasks that have sent messages.
  struct list_head waiting_tasks;

  struct task * receive; // The task waiting for a message.
  struct task * reply; // The task waiting for a reply.
  struct task * server; // The task currently handing a message.
} channel_t;

#endif
