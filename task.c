#include "task.h"

#include "kernel.h"
#include "utils.h"

#include <assert.h>

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
