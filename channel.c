/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <kevinmottashed@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * -Kevin Mottashed
 * ----------------------------------------------------------------------------
 */

#include "channel.h"

#include "manticore.h"

#include "syscall.h"
#include "system.h"
#include "kernel.h"
#include "heap.h"

#include <assert.h>

void channel_init(struct channel * channel)
{
  static int channel_id = 0;
  channel->id = channel_id++;
  list_init(&channel->waiting_tasks);
  channel->receive = NULL;
  channel->reply = NULL;
  channel->server = NULL;
}

void channel_send(struct channel * channel, void * msg, size_t len, void * reply, size_t * reply_len)
{
  running_task->channel = channel;
  running_task->channel_msg = msg;
  running_task->channel_len = len;
  running_task->channel_reply = reply;
  running_task->channel_reply_len = reply_len;
  SVC_CHANNEL_SEND();
}

size_t channel_recv(struct channel * channel, void * msg, size_t len)
{
  running_task->channel = channel;
  running_task->channel_msg = msg;
  running_task->channel_len = len;
  SVC_CHANNEL_RECV();
  return running_task->channel_len;
}

void channel_reply(struct channel * channel, void * msg, size_t len)
{
  running_task->channel = channel;
  running_task->channel_msg = msg;
  running_task->channel_len = len;
  SVC_CHANNEL_REPLY();
}
