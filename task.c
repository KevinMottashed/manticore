#include "task.h"

#include "manticore.h"

#include "kernel.h"
#include "utils.h"
#include "heap.h"

#include <assert.h>
#include <string.h>

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
  vector_init(&task->blocked);
  
  context_t * context = (context_t*)task->stackPointer;
  memset(context, 0, sizeof(*context)); // Most registers will start off as zero.
  context->R0 = (uint32_t)arg;
  context->LR = (uint32_t)&task_return;
  context->PC = (uint32_t)entry; // The program counter starts at the task entry point.
  context->xPSR.b.T = 1; // Enable Thumb mode.
  
  // Add the task to the queue of ready tasks.
  pqueue_push(&readyQueue, task, task->priority);
  
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
  
  vector_push_back(&task->blocked, blocked);
  
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
  
  vector_remove(&task->blocked, unblocked);
  
  // A task that was blocked on us should never have a higher priority.
  assert(task->priority >= unblocked->priority);
  
  if (unblocked->priority == task->priority &&
      task->provisionedPriority != task->priority)
  {
    // The unblocked task may be responsible for this tasks increased
    // priority. We need to reevaluate the priority.
    uint8_t newPriority = task->provisionedPriority;
    for (size_t i = 0; i < vector_size(&task->blocked); ++i)
    {
      task_t * blocked = vector_at(&task->blocked, i);
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
  else
  {
    // Check if our priority needs to be decreased.
    uint8_t newPriority = task->provisionedPriority;
    for (size_t i = 0; i < vector_size(&task->blocked); ++i)
    {
      task_t * blocked = vector_at(&task->blocked, i);
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

  if (task->state == STATE_READY)
  {
    // Reposition the task in the ready queue.
    pqueue_reschedule(&readyQueue, task, task->priority);
  }
  else if (task->state == STATE_MUTEX)
  {
    // We need to reschedule our position in the mutex queue.
    pqueue_reschedule(&task->mutex->queue, task, task->priority);
    
    // We also might need to reschedule the task that we're blocked on if
    // its priority changes as a result of our priority changing.
    bool reschedule = task_update_blocked(task->mutex->owner, task);
    if (reschedule)
    {
      task_reschedule(task->mutex->owner);
    }
  }
  else if (task->state == STATE_CHANNEL_SEND)
  {
    // We need to reschedule our position in the channels queue.
    pqueue_reschedule(&task->channel.c->sendQueue, task, task->priority);
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

  return;
}

void task_destroy(task_t * task)
{
  assert(task != NULL);
  vector_destroy(&task->blocked);
  heap_free(task);
}

void task_return(void * result)
{
  runningTask->waitResult = result;
  SVC_TASK_RETURN();
  // will never get here
  assert(false);
}
