#include "task.h"

#include "manticore.h"

#include "kernel.h"
#include "utils.h"
#include "heap.h"

#include <assert.h>
#include <string.h>

#define TASK_STACK_MAGIC (0x45e2c902)

static void task_return(void * result);

void task_init(struct task * task, task_entry_t entry, void * arg, void * stack, uint32_t stackSize, uint8_t priority)
{
  assert(stack != NULL);
  assert(stackSize >= sizeof(struct context));

  if (kernel_running)
  {
    // We can't be preempted while creating a new task.
    kernel_scheduler_disable();
  }

  static uint8_t task_id_counter = 0;
  task->id = task_id_counter++;
  task->state = STATE_READY;
  task->provisioned_priority = priority;
  task->priority = priority;
  task->stack_pointer = (uint32_t)stack + stackSize;
  task->stack_pointer &= ~0x7u; // The stack must be 8 byte aligned.
  task->stack_pointer -= sizeof(struct context);
  task->stack = stack;
  *(uint32_t*)task->stack = TASK_STACK_MAGIC;
  tree_init(&task->blocked);

  tree_init(&task->family);
  if (running_task != NULL)
  {
    tree_add_child(&running_task->family, &task->family);
  }

  struct context * context = (struct context*)task->stack_pointer;
  memset(context, 0, sizeof(*context)); // Most registers will start off as zero.
  context->R0 = (uint32_t)arg;
  context->LR = (uint32_t)task_return;
  context->PC = (uint32_t)entry; // The program counter starts at the task entry point.
  context->xPSR.b.T = 1; // Enable Thumb mode.

  // Add the task to the queue of ready tasks.
  list_push_back(&ready_tasks, &task->wait_node);

  if (kernel_running)
  {
    if (priority > task_get_priority(NULL))
    {
      // The new task has a higher priority than us. Yield to it.
      task_yield();
    }
    else
    {
      kernel_scheduler_enable();
    }
  }
}

void * task_wait(struct task ** task)
{
  running_task->wait = task;
  SVC_TASK_WAIT();
  return running_task->wait_result;
}

uint8_t task_get_priority(struct task * task)
{
  if (task == NULL)
  {
    return running_task->priority;
  }
  return task->priority;
}

void task_yield(void)
{
  SVC_YIELD();
}

void task_sleep(unsigned int seconds)
{
  // TODO: Test the max value. It won't work due to overflow so fix it.
  task_delay(seconds * 1000);
}

void task_delay(unsigned int ms)
{
  running_task->sleep = ms;
  SVC_SLEEP();
}

bool task_add_blocked(struct task * task, struct task * blocked)
{
  assert(task != NULL);
  assert(blocked != NULL);

  // Add the blocked task as a child in the tree
  tree_add_child(&task->blocked, &blocked->blocked);

  // Check if our priority needs to be increased.
  if (blocked->priority > task->priority)
  {
    task->priority = blocked->priority;
    return true;
  }
  return false;
}

bool task_remove_blocked(struct task * task, struct task * unblocked)
{
  assert(task != NULL);
  assert(unblocked != NULL);

  // Remove the unblocked task from the blocked tree.
  // We're no longer blocked on the parent task.
  tree_remove_child(&unblocked->blocked);

  // A task that was blocked on us should never have a higher priority.
  assert(task->priority >= unblocked->priority);

  if (unblocked->priority == task->priority &&
      task->provisioned_priority != task->priority)
  {
    // The unblocked task may be responsible for this tasks increased
    // priority. We need to reevaluate the priority.
    uint8_t newPriority = task->provisioned_priority;
    struct tree_head * node;
    tree_for_each_direct (node, &task->blocked)
    {
      struct task * blocked = container_of(node, struct task, blocked);
      assert(task->priority >= blocked->priority);
      newPriority = MAX(newPriority, blocked->priority);
    }
    if (newPriority != task->priority)
    {
      task->priority = newPriority;
      return true;
    }
  }
  return false;
}

bool task_update_blocked(struct task * task, struct task * blocked)
{
  // Check if our priority needs to be increased.
  if (blocked->priority > task->priority)
  {
    task->priority = blocked->priority;
    return true;
  }
  // Check if our priority needs to be decreased.
  else if (blocked->priority < task->priority)
  {
    uint8_t newPriority = task->provisioned_priority;
    struct tree_head * node;
    tree_for_each_direct (node, &task->blocked)
    {
      struct task * blocked = container_of(node, struct task, blocked);
      assert(task->priority >= blocked->priority);
      newPriority = MAX(newPriority, blocked->priority);
    }
    if (newPriority != task->priority)
    {
      task->priority = newPriority;
      return true;
    }
  }
  return false;
}

void task_reschedule(struct task * task)
{
  assert(task != NULL);
  assert(task->state != STATE_RUNNING);

  if (task->state == STATE_MUTEX)
  {
    // We might need to reschedule the task that we're blocked on if
    // its priority changes as a result of our priority changing.
    bool reschedule = task_update_blocked(task->mutex->owner, task);
    if (reschedule)
    {
      task_reschedule(task->mutex->owner);
    }
  }
  else if (task->state == STATE_CHANNEL_RPLY)
  {
    // We're waiting to receive a reply from another task.
    bool reschedule = task_update_blocked(task->channel->server, task);
    if (reschedule)
    {
      task_reschedule(task->channel->server);
    }
  }
  else if (task->state == STATE_WAIT && task->wait != NULL && *task->wait != NULL)
  {
    // Priority inheritence only applies when we're waiting on a specific child.
    bool reschedule = task_update_blocked(*task->wait, task);
    if (reschedule)
    {
      task_reschedule(*task->wait);
    }
  }

  return;
}

void task_destroy(struct task * task)
{
  assert(task != NULL);

  // Make sure no tasks are blocked on a dying task
  // or they'll be permanantly blocked.
  assert(tree_no_children(&task->blocked));

  // Remove the task from the family tree.
  tree_remove(&task->family);
  task->state = STATE_DEAD;
}

void task_return(void * result)
{
  running_task->wait_result = result;
  SVC_TASK_RETURN();
  // will never get here
  assert(false);
}

bool task_check(struct task * task)
{
  // Make sure the stack didn't overflow.
  return *(uint32_t*)task->stack == TASK_STACK_MAGIC;
}
