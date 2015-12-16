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
#include "list.h"

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

struct task * runningTask = NULL;
bool kernelRunning = false;

struct list_head ready_tasks;
static struct list_head sleeping_tasks;

static void kernel_update_sleep(unsigned int ticks);
static struct task * get_next_task(struct list_head * list);

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

// Internal OS tasks
#pragma data_alignment = 8
uint8_t idle_task_stack[128];
#pragma data_alignment = 8
uint8_t init_task_stack[128];

static __task void * kernel_task_idle(void * arg);
static __task void * kernel_task_init(void * arg);

void manticore_init(void)
{
  // Initialize the lists of ready/sleeping tasks.
  list_init(&ready_tasks);
  list_init(&sleeping_tasks);

  // Create the init task and set it as the initial running task.
  struct task * init = task_create(kernel_task_init, NULL, init_task_stack, sizeof(init_task_stack), 1);
  init->state = STATE_RUNNING;
  runningTask = init;
  list_remove(&init->wait_node);

  // Create the idle task.
  struct task * idle = task_create(kernel_task_idle, NULL, idle_task_stack, sizeof(idle_task_stack), 0);
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

    assert(task_check(runningTask));

    // Update how much time is left for each sleeping task to wake up.
    kernel_update_sleep(systickReload - kernelStartTicks + kernelTicks);

    // Handle the system call.
    kernel_handle_syscall(syscallValue);
    syscallValue = SYSCALL_NONE;

    // This is a priority round-robin scheduler. The highest
    // priority ready task will always run next and will never yield to
    // a lower priority task. The next task to run is the one at
    // the front of the priority queue.
    struct task * nextTask = get_next_task(&ready_tasks);
    assert(nextTask != NULL);
    list_remove(&nextTask->wait_node);

    // We now know the next task that will run. We need to find out if
    // there's a sleeping task that needs to wakeup before the time slice expires.
    unsigned int sleep = MAX_SYSTICK_RELOAD;
    if (nextTask != NULL)
    {
      // Have a peek at the next ready task. We need to use a time slice
      // if the next task has the same priority as the task we're about
      // to run. This ensures the CPU is shared between all the
      // highest priority tasks.
      struct task * temp = get_next_task(&ready_tasks);
      if (temp != NULL && temp->priority == nextTask->priority)
      {
        sleep = TIME_SLICE_TICKS;
      }

      struct list_head * node;
      list_for_each(node, &sleeping_tasks)
      {
        struct task * t = container_of(node, struct task, wait_node);
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
      struct list_head * node;
      list_for_each(node, &sleeping_tasks)
      {
        struct task * t = container_of(node, struct task, wait_node);
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

    // We have a task to schedule.
    runningTask = nextTask;
    runningTask->state = STATE_RUNNING;
    savedStackPointer = &runningTask->stackPointer;

    // Trigger a PendSV interrupt to return to task context.
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
    __DSB();
    __ISB();
  }
}

void kernel_update_sleep(unsigned int ticks)
{
  // Iterate through all the tasks to update the sleep time left.
  struct list_head * node;
  list_for_each_safe(node, &sleeping_tasks)
  {
    struct task * t = container_of(node, struct task, wait_node);
    if (ticks >= t->sleep)
    {
      // The task is ready. Remove it from the sleep vector.
      t->sleep = 0;
      t->state = STATE_READY;
      list_remove(&t->wait_node);
      list_push_back(&ready_tasks, &t->wait_node);
    }
    else
    {
      t->sleep -= ticks;
    }
  }
}

static struct task * get_next_task(struct list_head * list)
{
  struct task * next_task = NULL;
  struct list_head * node;
  list_for_each(node, list)
  {
    struct task * task = container_of(node, struct task, wait_node);
    if (next_task == NULL ||
        task->priority > next_task->priority)
    {
      next_task = task;
    }
  }
  return next_task;
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
  runningTask->state = STATE_READY;
  list_push_back(&ready_tasks, &runningTask->wait_node);
}

void kernel_handle_sleep(void)
{
  runningTask->state = STATE_SLEEP;
  runningTask->sleep *= SYSTICK_RELOAD_MS;
  list_push_back(&sleeping_tasks, &runningTask->wait_node);
}

void kernel_handle_mutex_lock(void)
{
  struct mutex * mutex = runningTask->mutex;
  assert(mutex != NULL);

  // Add the active task to the queue of tasks waiting for the mutex.
  runningTask->state = STATE_MUTEX;
  list_remove(&runningTask->wait_node);
  list_push_back(&mutex->waiting_tasks, &runningTask->wait_node);

  // The owner of the mutex is now blocking whoever tried to lock it.
  bool reschedule = task_add_blocked(mutex->owner, runningTask);
  if (reschedule)
  {
    task_reschedule(mutex->owner);
  }
}

void kernel_handle_mutex_unlock(void)
{
  struct mutex * mutex = runningTask->mutex;
  assert(mutex != NULL);

  // Unblock the next highest priority task waiting for this mutex.
  struct task * newOwner = get_next_task(&mutex->waiting_tasks);
  assert(newOwner != NULL);
  list_remove(&newOwner->wait_node);
  newOwner->state = STATE_READY;
  list_push_back(&ready_tasks, &newOwner->wait_node);

  // A task is still ready after unlocking a mutex.
  runningTask->state = STATE_READY;
  list_push_back(&ready_tasks, &runningTask->wait_node);

  // The previous owner of the mutex is no longer blocking tasks waiting
  // for the mutex. The new owner of the mutex is now blocking all those tasks.
  bool rescheduleOldOwner = false;
  bool rescheduleNewOwner = false;
  struct task * oldOwner = mutex->owner;
  mutex->owner = newOwner;
  rescheduleOldOwner = task_remove_blocked(oldOwner, newOwner);

  struct list_head * node;
  list_for_each(node, &mutex->waiting_tasks)
  {
    struct task * waiter = container_of(node, struct task, wait_node);
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
  channel_t * channel = runningTask->channel;

  struct task * recv = channel->receive;
  if (recv != NULL)
  {
    // There's a task waiting for a message.
    recv->state = STATE_READY;
    list_push_back(&ready_tasks, &recv->wait_node);
    channel->receive = NULL;
    channel->server = recv;

    // The sending task becomes reply blocked.
    runningTask->state = STATE_CHANNEL_RPLY;
    channel->reply = runningTask;

    // Copy the message from the sender to the receiver.
    assert(recv->channel_len >= runningTask->channel_len);
    memcpy(recv->channel_msg, runningTask->channel_msg, runningTask->channel_len);
    recv->channel_len = runningTask->channel_len;

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
    list_push_back(&channel->waiting_tasks, &runningTask->wait_node);
  }
}

void kernel_handle_channel_recv(void)
{
  struct task * send = get_next_task(&runningTask->channel->waiting_tasks);
  if (send != NULL)
  {
    // A task has already sent a message to this channel.
    runningTask->state = STATE_READY;
    list_push_back(&ready_tasks, &runningTask->wait_node);

    // The task that sent us a message is no longer waiting to send us a message.
    list_remove(&send->wait_node);

    // The task that sent the message becomes reply blocked.
    runningTask->channel->reply = send;
    runningTask->channel->server = runningTask;
    send->state = STATE_CHANNEL_RPLY;

    // Copy the message from the sender to the receiver.
    assert(runningTask->channel_len >= send->channel_len);
    memcpy(runningTask->channel_msg, send->channel_msg, send->channel_len);
    runningTask->channel_len = send->channel_len;

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
    runningTask->channel->receive = runningTask;
  }
}

void kernel_handle_channel_reply(void)
{
  channel_t * channel = runningTask->channel;
  void * msg = runningTask->channel_msg;
  size_t len = runningTask->channel_len;

  // Copy the reply to the task that sent us a message.
  struct task * replyTask = channel->reply;
  assert(replyTask != NULL);
  assert(replyTask->channel_len >= len);
  memcpy(replyTask->channel_reply, msg, len);
  *replyTask->channel_reply_len = len;
  channel->reply = NULL;
  channel->server = NULL;

  // The task we replied to becomes unblocked.
  replyTask->state = STATE_READY;
  list_push_back(&ready_tasks, &replyTask->wait_node);

  // The task that replied is still ready.
  runningTask->state = STATE_READY;
  list_push_back(&ready_tasks, &runningTask->wait_node);

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
  assert(tree_num_children(&runningTask->blocked) == 0 ||
         tree_num_children(&runningTask->blocked) == 1 &&
         container_of(tree_first_child(&runningTask->blocked), struct task, blocked)->state == STATE_WAIT);

  struct task * parent = container_of(runningTask->family.parent, struct task, family);

  // Check if our parent is waiting for us or any of it's children.
  if (parent->state == STATE_WAIT &&
      (parent->wait == NULL || *parent->wait == NULL || *parent->wait == runningTask))
  {
    // The parent is already waiting for us. Give it the return code.
    parent->waitResult = runningTask->waitResult;

    if (parent->wait != NULL && *parent->wait == runningTask)
    {
      // The parent task was waiting for us. We're no longer blocking it.
      task_remove_blocked(runningTask, parent);
    }

    if (parent->wait != NULL)
    {
      *parent->wait = runningTask;
    }

    // The parent task becomes ready and the returned task ceases to exist.
    parent->state = STATE_READY;
    list_push_back(&ready_tasks, &parent->wait_node);
    task_destroy(runningTask);
  }
  else
  {
    // The parent isn't waiting for us.
    // Go into the zombie state until the parent reaps us.
    runningTask->state = STATE_ZOMBIE;
  }
}

void kernel_handle_task_wait(void)
{
  // It makes no sense to call wait when we have no children.
  assert(tree_num_children(&runningTask->family) > 0);

  if (runningTask->wait != NULL && *runningTask->wait != NULL)
  {
    // We're waiting for a specific child task.
    struct task * child = *runningTask->wait;
    assert(child->family.parent == &runningTask->family);
    if (child->state == STATE_ZOMBIE)
    {
      // The child task has already returned.
      // Take the return value and destroy it.
      runningTask->state = STATE_READY;
      list_push_back(&ready_tasks, &runningTask->wait_node);
      runningTask->waitResult = child->waitResult;
      task_destroy(child);
    }
    else
    {
      // The child task hasn't returned yet. Wait for it.
      bool reschedule = task_add_blocked(child, runningTask);
      if (reschedule)
      {
        task_reschedule(child);
      }
      runningTask->state = STATE_WAIT;
    }
  }
  else
  {
    // We're waiting for any child task to return.
    // Check if there's already one that's returned.
    runningTask->state = STATE_WAIT;
    struct tree_head * node;
    tree_for_each_direct(node, &runningTask->family)
    {
      struct task * child = container_of(node, struct task, family);
      if (child->state == STATE_ZOMBIE)
      {
        // A child already terminated.
        if (runningTask->wait != NULL)
        {
          *runningTask->wait = child;
        }

        runningTask->state = STATE_READY;
        list_push_back(&ready_tasks, &runningTask->wait_node);
        runningTask->waitResult = child->waitResult;
        task_destroy(child);
        break;
      }
    }
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

static __task void * kernel_task_idle(void * arg)
{
  while (true)
  {
    // All that the idle task needs to do is wait for interrupts.
    __WFI();
  }
}

static __task void * kernel_task_init(void * arg)
{
  while (true)
  {
    // The init task is used to reap tasks that have returned.
    task_wait(NULL);
  }
}
