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

#include "syscall.h"
#include "system.h"

#include <assert.h>

void channel_init(channel_t * c)
{
  assert(c != NULL);
  static int channelId = 0;
  c->id = channelId++;
  
  pqueue_init(&c->sendQueue);
  c->receive = NULL;
  c->reply = NULL;
}

void channel_send(channel_t * c, void * msg, size_t len, void * reply, size_t * replyLen)
{
  // Disable the system tick ISR
  SysTick->CTRL = SysTick_CTRL_ENABLE_Msk;
  __DSB();
  __ISB();
  
  syscallContext.channel.c = c;
  syscallContext.channel.msg = msg;
  syscallContext.channel.len = len;
  syscallContext.channel.reply = reply;
  syscallContext.channel.replyLen = replyLen;
  
  asm("SVC #5");
}

void channel_recv(channel_t * c, void * msg, size_t len)
{
  // Disable the system tick ISR
  SysTick->CTRL = SysTick_CTRL_ENABLE_Msk;
  __DSB();
  __ISB();
  
  syscallContext.channel.c = c;
  syscallContext.channel.msg = msg;
  syscallContext.channel.len = len;
  
  asm("SVC #6");
}

void channel_reply(channel_t * c, void * msg, size_t len)
{
  // Disable the system tick ISR
  SysTick->CTRL = SysTick_CTRL_ENABLE_Msk;
  __DSB();
  __ISB();
  
  syscallContext.channel.c = c;
  syscallContext.channel.msg = msg;
  syscallContext.channel.len = len;
  
  asm("SVC #7");
}
