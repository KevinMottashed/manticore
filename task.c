#include "task.h"

#include "manticore.h"

#include "kernel.h"
#include "utils.h"
#include "heap.h"

#include <assert.h>
#include <string.h>

#define TASK_STACK_MAGIC (0x45e2c902)

static void task_return(void * result);

task_handle_t task_create(task_entry_t entry, void * arg, void * stack, uint32_t stackSize, uint8_t priority)
{
  assert(stack != NULL);
  assert(stackSize >= sizeof(context_t));

  if (kernelRunning)
  {
    // We can't be preempted while creating a new task.
    kernel_scheduler_disable();
  }

  task_t * task = heap_malloc(sizeof(*task));
  assert(task != NULL);

  static uint8_t taskIdCounter = 0;
  task->id = taskIdCounter++;
  task->state = STATE_READY;
  task->provisionedPriority = priority;
  task->priority = priority;
  task->stackPointer = (uint32_t)stack + stackSize;
  task->stackPointer &= ~0x7; // The stack must be 8 byte aligned.
  task->stackPointer -= sizeof(context_t);
  task->stack = stack;
  *(uint32_t*)task->stack = TASK_STACK_MAGIC;
  list_init(&task->blocking_head);
  list_init(&task->blocked_node);
  task->blocking_len = 0;

  context_t * context = (context_t*)task->stackPointer;
  memset(context, 0, sizeof(*context)); // Most registers will start off as zero.
  context->R0 = (uint32_t)arg;
  context->LR = (uint32_t)&task_return;
  context->PC = (uint32_t)entry; // The program counter starts at the task entry point.
  context->xPSR.b.T = 1; // Enable Thumb mode.

  // Add the task to the queue of ready tasks.
  list_push_back(&ready_tasks, &task->wait_node);

  if (kernelRunning)
  {
    kernel_scheduler_enable();
  }

  return task;
}

void * task_wait(task_handle_t task)
{
  runningTask->wait = task;
  SVC_TASK_WAIT();
  return runningTask->waitResult;
}

uint8_t task_get_priority(task_handle_t task)
{
  if (task == NULL)
  {
    return runningTask->priority;
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
  runningTask->sleep = ms;
  SVC_SLEEP();
}

bool task_add_blocked(task_t * task, task_t * blocked)
{
  assert(task != NULL);
  assert(blocked != NULL);

  ++task->blocking_len;
  list_push_back(&task->blocking_head, &blocked->blocked_node);

  // Check if our priority needs to be increased.
  if (blocked->priority > task->priority)
  {
    task->priority = blocked->priority;
    return true;
  }
  return false;
}

bool task_remove_blocked(task_t * task, task_t * unblocked)
{
  assert(task != NULL);
  assert(unblocked != NULL);

  --task->blocking_len;
  list_remove(&unblocked->blocked_node);

  // A task that was blocked on us should never have a higher priority.
  assert(task->priority >= unblocked->priority);

  if (unblocked->priority == task->priority &&
      task->provisionedPriority != task->priority)
  {
    // The unblocked task may be responsible for this tasks increased
    // priority. We need to reevaluate the priority.
    uint8_t newPriority = task->provisionedPriority;
    struct list_head * node;
    list_for_each (node, &task->blocking_head)
    {
      task_t * blocked = container_of(node, struct task_s, blocked_node);
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

bool task_update_blocked(task_t * task, task_t * blocked)
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
    uint8_t newPriority = task->provisionedPriority;
    struct list_head * node;
    list_for_each(node, &task->blocking_head)
    {
      task_t * blocked = container_of(node, struct task_s, blocked_node);
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

void task_reschedule(task_t * task)
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
    bool reschedule = task_update_blocked(task->channel.c->server, task);
    if (reschedule)
    {
      task_reschedule(task->channel.c->server);
    }
  }
  else if (task->state == STATE_WAIT)
  {
    bool reschedule = task_update_blocked(task->wait, task);
    if (reschedule)
    {
      task_reschedule(task->wait);
    }
  }

  return;
}

void task_destroy(task_t * task)
{
  assert(task != NULL);
  assert(task->blocking_len == 0);
  heap_free(task);
}

void task_return(void * result)
{
  runningTask->waitResult = result;
  SVC_TASK_RETURN();
  // will never get here
  assert(false);
}

bool task_check(task_t * task)
{
  // Make sure the stack didn't overflow.
  return *(uint32_t*)task->stack == TASK_STACK_MAGIC;
}
