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
  pqueue_init(&channel->sendQueue);
  channel->receive = NULL;
  channel->reply = NULL;
  channel->server = NULL;
  return channel;
}

void channel_send(channel_t * c, void * msg, size_t len, void * reply, size_t * replyLen)
{
  kernel_scheduler_disable();
  
  syscallContext.channel.c = c;
  syscallContext.channel.msg = msg;
  syscallContext.channel.len = len;
  syscallContext.channel.reply = reply;
  syscallContext.channel.replyLen = replyLen;
  SVC_CHANNEL_SEND();
}

void channel_recv(channel_t * c, void * msg, size_t len)
{
  kernel_scheduler_disable();
  
  syscallContext.channel.c = c;
  syscallContext.channel.msg = msg;
  syscallContext.channel.len = len;
  SVC_CHANNEL_RECV();
}

void channel_reply(channel_t * c, void * msg, size_t len)
{
  kernel_scheduler_disable();
  
  syscallContext.channel.c = c;
  syscallContext.channel.msg = msg;
  syscallContext.channel.len = len;
  SVC_CHANNEL_REPLY();
}
