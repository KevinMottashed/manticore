/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <kevinmottashed@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return. 
 * -Kevin Mottashed
 * ----------------------------------------------------------------------------
 */

#ifndef TASK_H
#define TASK_H

#include "system.h"
#include "mutex.h"
#include "channel.h"

#include <stdint.h>

#define MIN_PRIORITY (0)
#define MAX_PRIORITY (255)

typedef enum task_state_e
{
  STATE_RUNNING,
  STATE_READY,
  STATE_SLEEP,
  STATE_MUTEX,
  STATE_CHANNEL_SEND,
  STATE_CHANNEL_RECV,
  STATE_CHANNEL_RPLY,
} task_state_t;

// The is the context that will be saved and restored during context
// switches. The stack grows towards 0 which is why the registers
// saved by hardware appear in reverse order.
typedef struct context_s
{
  // The following registers are saved by software.
  uint32_t R8;
  uint32_t R9;
  uint32_t R10;
  uint32_t R11;
  uint32_t R4;
  uint32_t R5;
  uint32_t R6;
  uint32_t R7;
  
  // The following registers are saved by hardware when the ISR is entered.
  uint32_t R0;
  uint32_t R1;
  uint32_t R2;
  uint32_t R3;
  uint32_t R12;
  uint32_t LR;
  uint32_t PC;
  xPSR_Type xPSR;
} context_t;

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

typedef struct task_s
{
  uint32_t id;
  uint32_t stackPointer;
  task_state_t state;
  uint8_t priority;
  
  // The context that needs to be saved when in a blocked state.
  // A task can only be in one state at a time so a union is used to save space.
  union
  {
    unsigned int sleep;
    channel_context_t channel;
  };
} task_t;

#endif