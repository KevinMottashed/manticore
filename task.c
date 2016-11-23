#include "task.h"

#include "manticore.h"

#include "kernel.h"
#include "utils.h"

#include <assert.h>
#include <string.h>

#define TASK_STACK_MAGIC (0x45e2c902)

static void task_return(void * result);

void task_init(struct task * task, task_entry_t entry, void * arg, void * stack, uint32_t stack_size, uint8_t priority)
{
  assert(stack != NULL);
  assert(((uintptr_t)stack & 7) == 0); // The stack must be 8 byte aligned.
  assert(stack_size >= sizeof(struct context));

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
  task->stack_pointer = (uint32_t)stack + stack_size;
  task->stack_pointer -= sizeof(struct context);
  task->stack = stack;
  *(uint32_t*)task->stack = TASK_STACK_MAGIC;
  task->blocked = NULL;
  pqueue_init(&task->blocking, pqueue_blocking_compare);
  list_init(&task->blocking_node);
  list_init(&task->sleep_node);
  task->sleep = 0;

  tree_init(&task->family);
  if (running_task != NULL)
  {
    tree_add_child(&running_task->family, &task->family);
  }

  task->waiting = NULL;
  list_init(&task->wait_node);

  struct context * context = (struct context*)task->stack_pointer;
  memset(context, 0, sizeof(*context)); // Most registers will start off as zero.
  context->R0 = (uint32_t)arg;
  context->LR = (uint32_t)task_return;
  context->PC = (uint32_t)entry; // The program counter starts at the task entry point.
  context->xPSR.b.T = 1; // Enable Thumb mode.

  // Add the task to the queue of ready tasks.
  task_wait_on(task, &ready_tasks);

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

void task_add_blocked(struct task * task, struct task * blocked)
{
  assert(task != NULL);
  assert(blocked != NULL);
  assert(blocked->blocked == NULL);
  assert(list_empty(&blocked->blocking_node));

  // Start blocking the task.
  blocked->blocked = task;
  pqueue_push(&task->blocking, &blocked->blocking_node);

  // Walk through the chain of tasks that we're blocked on
  // and increase their priority if needed.
  while (task && blocked->priority > task->priority)
  {
    task->priority = blocked->priority;
    if (task->waiting)
      pqueue_increase(task->waiting, &task->wait_node);
    if (task->blocked)
      pqueue_increase(&task->blocked->blocking, &task->blocking_node);
    task = task->blocked;
  }
}

void task_remove_blocked(struct task * task, struct task * unblocked)
{
  assert(task != NULL);
  assert(unblocked != NULL);
  assert(unblocked->blocked == task);
  assert(!list_empty(&unblocked->blocking_node));

  // A task that was blocked on us should never have a higher priority.
  assert(task->priority >= unblocked->priority);

  // Stop blocking the task.
  unblocked->blocked = NULL;
  list_remove(&unblocked->blocking_node);

  // Walk through the chain of tasks that we're blocked on
  // and decrease their priority if needed.
  while (task) {
    // Our next priority is the maximum of our provisioned priority and
    // the priority of the tasks that are blocked on us.
    uint8_t priority = task->provisioned_priority;
    if (!pqueue_empty(&task->blocking)) {
      struct task * high = task_from_blocking_node(pqueue_peek(&task->blocking));
      if (high->priority > priority)
        priority = high->priority;
    }

    assert(priority <= task->priority);
    if (priority < task->priority) {
      task->priority = priority;
      if (task->blocked)
        pqueue_decrease(&task->blocked->blocking, &task->blocking_node);
      if (task->waiting)
        pqueue_decrease(task->waiting, &task->wait_node);
      task = task->blocked;
    }
    else {
      // The priority didn't need to be decreased. When
      // this happens we know that everything further down the chain
      // also doesn't need to be decreased.
      break;
    }
  }
}

void task_wait_on(struct task * task, struct pqueue * pqueue)
{
  assert(task);
  assert(pqueue);
  assert(task->waiting == NULL);
  assert(list_empty(&task->wait_node));
  task->waiting = pqueue;
  pqueue_push(pqueue, &task->wait_node);
}

void task_stop_waiting(struct task * task)
{
  assert(task);
  assert(task->waiting);
  task->waiting = NULL;
  list_remove(&task->wait_node);
}

void task_destroy(struct task * task)
{
  assert(task != NULL);

  // Make sure the dying tasks isn't blocking other tasks
  // or they'll be permanantly blocked.
  assert(pqueue_empty(&task->blocking));

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

bool pqueue_wait_compare(struct list_head * a, struct list_head * b)
{
  struct task * ta = container_of(a, struct task, wait_node);
  struct task * tb = container_of(b, struct task, wait_node);
  return ta->priority > tb->priority;
}

bool pqueue_blocking_compare(struct list_head * a, struct list_head * b)
{
  struct task * ta = container_of(a, struct task, blocking_node);
  struct task * tb = container_of(b, struct task, blocking_node);
  return ta->priority > tb->priority;
}
