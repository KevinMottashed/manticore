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

channel_handle_t channel_create(void)
{
  static int channelId = 0;

  channel_t * channel = heap_malloc(sizeof(*channel));
  channel->id = channelId++;
  list_init(&channel->waiting_tasks);
  channel->receive = NULL;
  channel->reply = NULL;
  channel->server = NULL;
  return channel;
}

void channel_send(channel_handle_t channel, void * msg, size_t len, void * reply, size_t * replyLen)
{
  runningTask->channel = channel;
  runningTask->channel_msg = msg;
  runningTask->channel_len = len;
  runningTask->channel_reply = reply;
  runningTask->channel_reply_len = replyLen;
  SVC_CHANNEL_SEND();
}

size_t channel_recv(channel_handle_t channel, void * msg, size_t len)
{
  runningTask->channel = channel;
  runningTask->channel_msg = msg;
  runningTask->channel_len = len;
  SVC_CHANNEL_RECV();
  return runningTask->channel_len;
}

void channel_reply(channel_handle_t channel, void * msg, size_t len)
{
  runningTask->channel = channel;
  runningTask->channel_msg = msg;
  runningTask->channel_len = len;
  SVC_CHANNEL_REPLY();
}
