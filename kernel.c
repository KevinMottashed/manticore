/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <kevinmottashed@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return. 
 * -Kevin Mottashed
 * ----------------------------------------------------------------------------
 */

#include "kernel.h"
   
#include "manticore.h"

#include "task.h"
#include "system.h"
#include "syscall.h"
#include "utils.h"
#include "clock.h"
#include "mutex.h"
#include "heap.h"
#include "pqueue.h"
#include "vector.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

// The system clock is divided by 8 and drives the SysTick timer.
#define SYSTICK_RELOAD_MS (SYSTEM_CLOCK / 8 / 1000)

 // The counter is 24 bits.
#define MAX_SYSTICK_RELOAD (0xFFFFFF)

// The time slice in milliseconds
#define TIME_SLICE_MS (10)

// The number of SysTick ticks in a time slice
#define TIME_SLICE_TICKS (TIME_SLICE_MS * SYSTICK_RELOAD_MS)

// This is the aproximate number of systicks spent in the kernel
// while the systick timer is being reset.
#define KERNEL_SYSTICK_DISABLE (2)

volatile uint8_t syscallValue;
volatile uint32_t * savedStackPointer;

task_t * runningTask = NULL;
pqueue_t readyQueue;
bool kernelRunning = false;

static vector_t sleepVector;

static void kernel_update_sleep(unsigned int ticks);

// Handle the various system calls
static void kernel_handle_syscall(uint8_t value);
static void kernel_handle_yield(void);
static void kernel_handle_sleep(void);
static void kernel_handle_mutex_lock(void);
static void kernel_handle_mutex_unlock(void);
static void kernel_handle_channel_send(void);
static void kernel_handle_channel_recv(void);
static void kernel_handle_channel_reply(void);
static void kernel_handle_task_return(void);
static void kernel_handle_task_wait(void);

void manticore_init(void)
{
  // Initialize the ready queue and the vector of sleeping tasks.
  pqueue_init(&readyQueue);
  vector_init(&sleepVector);
}

void manticore_main(void)
{
  kernelRunning = true;
  
  unsigned int kernelTicks = 0;
  unsigned int systickReload = 0xFFFFFF;
  
  while (true)
  {
    // The systick value when entering the kernel.
    int kernelStartTicks = SysTick->VAL;
    
    if (runningTask != NULL)
    {
      assert(task_check(runningTask));
    }
    
    // Update how much time is left for each sleeping task to wake up.
    kernel_update_sleep(systickReload - kernelStartTicks + kernelTicks);
    
    // Handle the system call.
    kernel_handle_syscall(syscallValue);
    syscallValue = SYSCALL_NONE;
    
    // This is a priority round-robin scheduler. The highest
    // priority ready task will always run next and will never yield to
    // a lower priority task. The next task to run is the one at 
    // the front of the priority queue.
    task_t * nextTask = pqueue_pop(&readyQueue);
    
    // We now know the next task that will run. We need to find out if
    // there's a sleeping task that needs to wakeup before the time slice expires.
    unsigned int sleep = MAX_SYSTICK_RELOAD;
    if (nextTask != NULL)
    {
      // Have a peek at the next ready task. We need to use a time slice
      // if the next task has the same priority as the task we're about
      // to run. This ensures the CPU is shared between all the 
      // highest priority tasks.
      task_t * temp = pqueue_top(&readyQueue);
      assert(temp != NULL);
      if (temp != NULL && temp->priority == nextTask->priority)
      {
        sleep = TIME_SLICE_TICKS;
      }
      for (int i = 0; i < vector_size(&sleepVector); ++i)
      {
        task_t * t = vector_at(&sleepVector, i);
        if (t->priority > nextTask->priority)
        {
          // A higher priority sleeping task can reduce the sleep time
          // to less than the time slice.
          sleep = MIN(sleep, t->sleep);
        }
        else if (t->priority == nextTask->priority)
        {
          // An equal priority sleeping task can reduce the sleep time
          // but still needs to respect the time slice.
          unsigned int temp = MIN(sleep, t->sleep);
          sleep = MAX(temp, TIME_SLICE_TICKS);
        }
      }
    }
    else
    {
      // We have no ready tasks.
      // Sleep until the next sleeping task needs wakes up.
      for (int i = 0; i < vector_size(&sleepVector); ++i)
      {
        task_t * t = vector_at(&sleepVector, i);
        sleep = MIN(sleep, t->sleep);
      }
    }
    
    // We don't want to sleep for less than 1ms.
    // When the systick timer is enabled we don't want it to immediatly fire.
    // We need to make it back to the user task before the IRQ goes off.
    // This will result in sleep() calls lasting up to 1ms longer than expected.
    // The other option is to wake up early but I think that sleeping longer is better.
    // In my experience sleep() calls are used to wait at least x amount of time.
    // For example:
    // 1. Start an operation.
    // 2. sleep()
    // 3. Check the status. The operation should be done by now or its failed.
    sleep = MAX(sleep, SYSTICK_RELOAD_MS);
    systickReload = sleep;
    
    // Remember the amount of time spent in the kernel.
    kernelTicks = kernelStartTicks - SysTick->VAL + KERNEL_SYSTICK_DISABLE;
    
    // Restart the systick timer.  
    SysTick->LOAD = systickReload;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_TICKINT_Msk;
    __DSB();
    __ISB();
    SysTick->LOAD = 0;
    __DSB();
    __ISB();
    
    if (nextTask != NULL)
    {
      // We have a task to schedule.
      runningTask = nextTask;
      runningTask->state = STATE_RUNNING;
      savedStackPointer = &runningTask->stackPointer;
      
      // Trigger a PendSV interrupt to return to task context.
      SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
      __DSB();
      __ISB();
    }
    else
    {
      // We have no tasks to schedule so we wait for the next SysTick interrupt.
      // The SysTick handler will run but will immediatly return.
      // See the comment in the SysTick handler.
      // Some user ISRs could run here. Hence the loop.
      // This loop is essentially the idle task.
      runningTask = NULL;
      do
      {
        __WFI();
      } while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0);
    }
  }
}

void kernel_update_sleep(unsigned int ticks)
{
  // Iterate through all the tasks to update the sleep time left.
  for (int i = 0; i < vector_size(&sleepVector);)
  {
    task_t * t = vector_at(&sleepVector, i);
    if (ticks >= t->sleep)
    {
      // The task is ready. Remove it from the sleep vector.
      t->sleep = 0;
      t->state = STATE_READY;
      pqueue_push(&readyQueue, t, t->priority);
      vector_erase(&sleepVector, i);
    }
    else
    {
      t->sleep -= ticks;
      ++i;
    }
  }  
}

void kernel_handle_syscall(uint8_t value)
{
  switch (value)
  {
  case SYSCALL_NONE:
  case SYSCALL_YIELD:
    kernel_handle_yield();
    break;
  case SYSCALL_SLEEP:
    kernel_handle_sleep();
    break;
  case SYSCALL_MUTEX_LOCK:
    kernel_handle_mutex_lock();
    break;
  case SYSCALL_MUTEX_UNLOCK:
    kernel_handle_mutex_unlock();
    break;
  case SYSCALL_CHANNEL_SEND:
    kernel_handle_channel_send();
    break;
  case SYSCALL_CHANNEL_RECV:
    kernel_handle_channel_recv();
    break;
  case SYSCALL_CHANNEL_REPLY:
    kernel_handle_channel_reply();
    break;
  case SYSCALL_TASK_RETURN:
    kernel_handle_task_return();
    break;
  case SYSCALL_TASK_WAIT:
    kernel_handle_task_wait();
    break;    
  default:
    assert(false);
  }
}

void kernel_handle_yield(void)
{
  // There's nothing special to do here. One of the following occured:
  // 1. The time slice expired.
  // 2. A task yielded the remainder of its time slice.
  // 3. The systick expired but no tasks were running.
  // Either way, the active task is no longer running.
  if (runningTask != NULL)
  {
    runningTask->state = STATE_READY;
    pqueue_push(&readyQueue, runningTask, runningTask->priority);
  }
}

void kernel_handle_sleep(void)
{
  runningTask->state = STATE_SLEEP;
  runningTask->sleep *= SYSTICK_RELOAD_MS;
  vector_push_back(&sleepVector, runningTask);
}

void kernel_handle_mutex_lock(void)
{
  mutex_t * mutex = runningTask->mutex;
  assert(mutex != NULL);
  
  // Add the active task to the queue of tasks waiting for the mutex.
  runningTask->state = STATE_MUTEX;
  pqueue_push(&mutex->queue, runningTask, runningTask->priority);
  
  // The owner of the mutex is now blocking whoever tried to lock it.
  bool reschedule = task_add_blocked(mutex->owner, runningTask);
  if (reschedule)
  {
    task_reschedule(mutex->owner);
  }
}

void kernel_handle_mutex_unlock(void)
{
  mutex_t * mutex = runningTask->mutex;
  assert(mutex != NULL);
  
  // Unblock the next highest priority task waiting for this mutex.
  task_t * newOwner = pqueue_pop(&mutex->queue);
  assert(newOwner != NULL);
  newOwner->state = STATE_READY;
  pqueue_push(&readyQueue, newOwner, newOwner->priority);
  
  // A task is still ready after unlocking a mutex.
  runningTask->state = STATE_READY;
  pqueue_push(&readyQueue, runningTask, runningTask->priority);
  
  // The previous owner of the mutex is no longer blocking tasks waiting
  // for the mutex. The new owner of the mutex is now blocking all those tasks.
  bool rescheduleOldOwner = false;
  bool rescheduleNewOwner = false;
  task_t * oldOwner = mutex->owner;
  mutex->owner = newOwner;
  rescheduleOldOwner = task_remove_blocked(oldOwner, newOwner);
  for (int i = 0; i < pqueue_size(&mutex->queue); ++i)
  {
    task_t * waiter = pqueue_at(&mutex->queue, i);
    rescheduleOldOwner |= task_remove_blocked(oldOwner, waiter);
    rescheduleNewOwner |= task_add_blocked(newOwner, waiter);
  }
  if (rescheduleOldOwner)
  {
    task_reschedule(oldOwner);
  }
  if (rescheduleNewOwner)
  {
    task_reschedule(newOwner);
  }
}

void kernel_handle_channel_send(void)
{
  // Save the message and channel
  channel_t * channel = runningTask->channel.c;
      
  task_t * recv = channel->receive;
  if (recv != NULL)
  {
    // There's a task waiting for a message.
    recv->state = STATE_READY;
    pqueue_push(&readyQueue, recv, recv->priority);
    channel->receive = NULL;
    channel->server = recv;

    // The sending task becomes reply blocked.
    runningTask->state = STATE_CHANNEL_RPLY;
    channel->reply = runningTask;
        
    // Copy the message from the sender to the receiver.
    assert(recv->channel.len >= runningTask->channel.len);
    memcpy(recv->channel.msg, runningTask->channel.msg, runningTask->channel.len);
    recv->channel.len = runningTask->channel.len;
    
    // The receiver is now blocking the sender;
    bool reschedule = task_add_blocked(recv, runningTask);
    if (reschedule)
    {
      task_reschedule(recv);
    }
  }
  else
  {
    // No task is waiting for a message.
    runningTask->state = STATE_CHANNEL_SEND;
    pqueue_push(&runningTask->channel.c->sendQueue, runningTask, runningTask->priority);
  }
}

void kernel_handle_channel_recv(void)
{
  task_t * send = pqueue_pop(&runningTask->channel.c->sendQueue);
  if (send != NULL)
  {
    // A task has already sent a message to this channel.
    runningTask->state = STATE_READY;
    pqueue_push(&readyQueue, runningTask, runningTask->priority);
        
    // The task that sent the message becomes reply blocked.
    runningTask->channel.c->reply = send;
    runningTask->channel.c->server = runningTask;
    send->state = STATE_CHANNEL_RPLY;
    
    // Copy the message from the sender to the receiver.
    assert(runningTask->channel.len >= send->channel.len);
    memcpy(runningTask->channel.msg, send->channel.msg, send->channel.len);
    runningTask->channel.len = send->channel.len;
    
    // The receiver is now blocking the sender.
    bool reschedule = task_add_blocked(runningTask, send);
    if (reschedule)
    {
      task_reschedule(runningTask);
    }
  }
  else
  {
    // No one has sent us a message :-(
    // We become receive blocked.
    runningTask->state = STATE_CHANNEL_RECV;
    runningTask->channel.c->receive = runningTask;
  }
}

void kernel_handle_channel_reply(void)
{
  channel_t * channel = runningTask->channel.c;
  void * msg = runningTask->channel.msg;
  size_t len = runningTask->channel.len;
  
  // Copy the reply to the task that sent us a message.
  task_t * replyTask = channel->reply;
  assert(replyTask != NULL);
  assert(replyTask->channel.len >= len);
  memcpy(replyTask->channel.reply, msg, len);
  *replyTask->channel.replyLen = len;
  channel->reply = NULL;
  channel->server = NULL;
  
  // The task we replied to becomes unblocked.
  replyTask->state = STATE_READY;
  pqueue_push(&readyQueue, replyTask, replyTask->priority);
  
  // The task that replied is still ready.
  runningTask->state = STATE_READY;      
  pqueue_push(&readyQueue, runningTask, runningTask->priority);
  
  // The task that replied is now longer blocking the sender.
  bool reschedule = task_remove_blocked(runningTask, replyTask);
  if (reschedule)
  {
    task_reschedule(runningTask);
  }
}

void kernel_handle_task_return(void)
{
  // When a task returns there should be exactly 0 or 1 tasks blocked on it.
  // If there's a task blocked on this task then it should be in the wait state.
  // It makes no sense for a task to return while holding a mutex or doing
  // something else that would cause a another task to block on it.
  // If no tasks are blocked on this task then it will go in the zombie state
  // until some other task retrieves the return code.
  assert(vector_size(&runningTask->blocked) == 0 ||
         (vector_size(&runningTask->blocked) == 1 &&
         ((task_t*)vector_at(&runningTask->blocked, 0))->state == STATE_WAIT));
  
  if (vector_size(&runningTask->blocked) == 1)
  {
    // Another task is already waiting for us. Give it the return code.
    task_t * task = vector_at(&runningTask->blocked, 0);
    task->waitResult = runningTask->waitResult;
    
    // The other task becomes ready and the returned task ceases to exist.
    task->state = STATE_READY;
    pqueue_push(&readyQueue, task, task->priority);
    task_destroy(runningTask);
  }
  else
  {
    // No one is waiting for this task. Go into the zombie state until
    // someone retrieves the return code.
    runningTask->state = STATE_ZOMBIE;
  }
}

void kernel_handle_task_wait(void)
{
  assert(runningTask->wait != NULL);
  
  task_t * task = runningTask->wait;
  if (task->state == STATE_ZOMBIE)
  {
    // The other task has already returned. 
    // Take the return value and destroy it.
    runningTask->state = STATE_READY;
    pqueue_push(&readyQueue, runningTask, runningTask->priority);
    runningTask->waitResult = task->waitResult;
    task_destroy(task);
  }
  else
  {
    // The other task hasn't returned yet. Wait for it.
    bool reschedule = task_add_blocked(task, runningTask);
    if (reschedule)
    {
      task_reschedule(task);
    }
    runningTask->state = STATE_WAIT;
  }
}

/*
 * See section B2.5 of the ARM architecture reference manual.
 * Updates to the SCS registers require the use of DSB/ISB instructions.
 * The SysTick and SCB are part of the SCS (System control space).
 */
void kernel_scheduler_disable(void)
{
  // Disable the system tick ISR
  SysTick->CTRL = SysTick_CTRL_ENABLE_Msk;
  __DSB();
  __ISB();
}

void kernel_scheduler_enable(void)
{
  // Reenable the systick ISR.
  SysTick->CTRL = SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
  __DSB();
  __ISB();
  if (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)
  {
    // Trigger the systick ISR if the timer expired
    // while the ISR was disabled.
    SCB->ICSR = SCB_ICSR_PENDSTSET_Msk;
    __DSB();
    __ISB();
  }
}
