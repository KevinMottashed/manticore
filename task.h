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
#include "syscall.h"
#include "list.h"
#include "tree.h"

#include <stdint.h>
#include <stdbool.h>

#define MIN_PRIORITY (0)
#define MAX_PRIORITY (255)

enum task_state
{
  STATE_RUNNING,
  STATE_READY,
  STATE_SLEEP,
  STATE_MUTEX,
  STATE_CHANNEL_SEND,
  STATE_CHANNEL_RECV,
  STATE_CHANNEL_RPLY,
  STATE_ZOMBIE,
  STATE_WAIT,
  STATE_DEAD
};

// The is the context that will be saved and restored during context
// switches. The stack grows towards 0.
struct context
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
};

struct task
{
  uint32_t id;
  uint32_t stack_pointer;
  void * stack;
  enum task_state state;

  // The provisioned and real priorities. The real priority is updated
  // via priority inheritence when other tasks block/unblock on this task.
  uint8_t provisioned_priority;
  uint8_t priority;

  // The task that we're blocked on.
  struct task * blocked;

  // The priority queue of tasks that we're blocking.
  struct pqueue blocking;
  struct list_head blocking_node;

  // The tree that represents the parent/child relationship between tasks.
  struct tree_head family;

  // The priority queue that we're waiting on.
  struct pqueue * waiting;
  struct list_head wait_node;

  // List node used when a task did a system call that can sleep or timeout.
  // The task will become ready after the sleep or timeout elapses.
  struct list_head sleep_node;

  // The time in systicks that we need to sleep for before becoming ready.
  unsigned int sleep;

  // The context that needs to be saved when in a blocked state.
  // A task can only be in one state at a time so a union is used to save space.
  union
  {
    struct mutex_context
    {
      struct mutex * mutex; // The mutex we're waiting on.
      bool mutex_locked;    // False when timed lock fails.
    };

    struct channel_context
    {
      // The channel that we're block on
      struct channel * channel;

      // Used for storing the send/recv buffer.
      void * channel_msg;
      size_t channel_len;

      // Used for storing the reply buffer.
      void * channel_reply;
      size_t * channel_reply_len;
    };

    // The task we're waiting on and the result it returned.
    struct
    {
      struct task ** wait;
      void * wait_result;
    };
  };
};

// A task has (un)blocked on this task. This will add/remove the task to the list
// of blocked tasks and will return true if the tasks priority has changed.
// TODO Add a version that takes a list/pqueue. These shouldn't be called in a loop (MUTEX).
void task_add_blocked(struct task * task, struct task * blocked);
void task_remove_blocked(struct task * task, struct task * unblocked);

// Start and stop waiting on a priority queue.
void task_wait_on(struct task * task, struct pqueue * pqueue);
void task_stop_waiting(struct task * task);

// Destroy a task. Release all allocated resources.
void task_destroy(struct task * task);

// Perform a sanity check on the task.
bool task_check(struct task * task);

// Compares the priority of 2 tasks given their wait_nodes.
bool pqueue_wait_compare(struct list_head * a, struct list_head * b);
bool pqueue_blocking_compare(struct list_head * a, struct list_head * b);

#define task_from_wait_node(node) container_of((node), struct task, wait_node)
#define task_from_blocking_node(node) container_of((node), struct task, blocking_node)

#endif
