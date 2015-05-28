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
#include "vector.h"
#include "syscall.h"

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

typedef struct task_s
{
  uint32_t id;
  uint32_t stackPointer;
  task_state_t state;
  
  // The provisioned and real priorities. The real priority is updated
  // via priority inheritence when other tasks block/unblock on this task.
  uint8_t provisionedPriority;
  uint8_t priority;
  
  // The tasks that are blocked on this task.
  struct vector_s blocked;
  
  // The context that needs to be saved when in a blocked state.
  // A task can only be in one state at a time so a union is used to save space.
  union
  {
    unsigned int sleep;
    struct mutex_s * mutex;
    struct channel_context_s
    {
      // The channel that we're block on
      struct channel_s * c;
  
      // Used for storing the send/recv buffer.
      void * msg;
      size_t len;
  
      // Used for storing the reply buffer.
      void * reply;
      size_t * replyLen;
    } channel;
  };
} task_t;

// A task has (un)blocked on this task. This will add/remove the task to the list
// of blocked tasks and will return true if the tasks priority has changed.
bool task_add_blocked(task_t * task, task_t * blocked);
bool task_remove_blocked(task_t * task, task_t * unblocked);

// Notifies <task> that the priority of <blocked> has changed.
bool task_update_blocked(task_t * task, task_t * blocked);

// Reschedule a task. A task needs to be rescheduled after its priority
// has changed. This should be called whenever task_add_blocked(),
// task_remove_blocked() or task_update_blocked() returns true.
void task_reschedule(task_t * task);

#endif