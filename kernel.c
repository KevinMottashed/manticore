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

struct task * running_task = NULL;
bool kernel_running = false;

struct pqueue ready_tasks;
static struct list_head sleeping_tasks;

static struct task * get_next_task(struct list_head * list);

__root void systick_handle(void);

// Handle the various system calls
__root void svc_handle(uint8_t value);
static void svc_handle_yield(void);
static void svc_handle_sleep(void);
static void svc_handle_mutex_lock(void);
static void svc_handle_mutex_unlock(void);
static void svc_handle_channel_send(void);
static void svc_handle_channel_recv(void);
static void svc_handle_channel_reply(void);
static void svc_handle_task_return(void);
static void svc_handle_task_wait(void);

// Internal OS tasks
#pragma data_alignment = 8
static uint8_t idle_task_stack[128];
#pragma data_alignment = 8
static uint8_t init_task_stack[128];

static struct task idle_task;
static struct task init_task;
static __task void * kernel_task_idle(void * arg);
static __task void * kernel_task_init(void * arg);
static void schedule(void);

void manticore_init(void)
{
  // These are undefined after reset so give them sane values.
  SysTick->LOAD = 0;
  SysTick->VAL = 0;

  // Initialize the lists of ready/sleeping tasks.
  pqueue_init(&ready_tasks, pqueue_wait_compare);
  list_init(&sleeping_tasks);

  // Create the init and idle tasks.
  // Pretend that the init task is running so that it becomes the parent
  // for future tasks.
  task_init(&init_task, kernel_task_init, NULL, init_task_stack, sizeof(init_task_stack), 1);
  running_task = &init_task;
  task_init(&idle_task, kernel_task_idle, NULL, idle_task_stack, sizeof(idle_task_stack), 0);
}

void manticore_main(void)
{
  running_task = NULL;
  kernel_running = true;

  // Figure out which task will run first.
  // Technically the SysTick is running at this point so the first task
  // will lose a bit of its time slice. The amount should be negligable.
  schedule();

  // Remove the saved context from the tasks stack.
  struct context * context = (struct context*)running_task->stack_pointer;
  running_task->stack_pointer += sizeof(*context);

  // Set the process stack pointer.
  __set_PSP(running_task->stack_pointer);

  // Save the tasks R0, LR and PC in registers.
  register uint32_t r0 = context->R0;
  register uint32_t lr = context->LR;
  register uint32_t pc = context->PC;

  // Switch to using the process stack.
  __set_CONTROL(0x2);
  __ISB();

  // Reset the main stack pointer to what it was at power on to reclaim space.
  extern uint32_t __vector_table;
  __set_MSP(__vector_table);

  // Push the argument and PC so that we can jump to the task later.
  // 2 PUSH instructions are used in case pc/r0 aren't in the right order.
  asm("PUSH {%[pc]}" : : [pc] "r" (pc));
  asm("PUSH {%[r0]}" : : [r0] "r" (r0));

  // Set LR to the task's exit function and set everything else to 0.
  // I don't think anything requires all registers be set to zero but
  // it happens for all tasks
  asm("MOV LR, %[lr]" : : [lr] "r" (lr));
  asm("MOVS R1, #0");
  asm("MOVS R2, #0");
  asm("MOVS R3, #0");
  asm("MOVS R4, #0");
  asm("MOVS R5, #0");
  asm("MOVS R6, #0");
  asm("MOVS R7, #0");
  asm("MOV R8, R1");
  asm("MOV R9, R1");
  asm("MOV R10, R1");
  asm("MOV R11, R1");
  asm("MOV R12, R1");

  // Pop the argument and PC to jump to the task.
  asm("POP {R0, PC}");
  assert(false);
}

static void update_sleep_ticks(uint32_t ticks)
{
  // Iterate through all the tasks to update the sleep time left.
  struct list_head * node;
  list_for_each_safe(node, &sleeping_tasks)
  {
    struct task * t = container_of(node, struct task, sleep_node);
    if (ticks >= t->sleep)
    {
      if (t->state == STATE_MUTEX)
      {
        // In this case mutex_timed_lock() timed out.
        t->mutex_locked = false;
        task_stop_waiting(t);

        // The task waiting for the mutex is no longer blocked on the mutex owner.
        task_remove_blocked(t->mutex->owner, t);
      }

      // The task is ready. Move it from the sleep list to the ready list.
      t->sleep = 0;
      t->state = STATE_READY;
      list_remove(&t->sleep_node);
      task_wait_on(t, &ready_tasks);
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

static void schedule(void)
{
  // Update how many ticks are left before the sleeping tasks wake up.
  uint32_t systick_load = SysTick->LOAD;
  uint32_t systick_val = SysTick->VAL;
  bool systick_fired = SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk;
  uint32_t ticks = systick_load - systick_val;
  if (systick_fired)
    ticks += systick_load;
  update_sleep_ticks(ticks);

  // This is a priority round-robin scheduler. The highest
  // priority ready task will always run next and will never yield to
  // a lower priority task. The next task to run is the one at
  // the front of the priority queue.
  struct task * next_task = task_from_wait_node(pqueue_peek(&ready_tasks));
  task_stop_waiting(next_task);

  // We now know the next task that will run. We need to find out if
  // there's a blocked task that needs to wakeup before the time slice expires.
  uint32_t task_ticks = MAX_SYSTICK_RELOAD;

  // Have a peek at the next ready task. We need to use a time slice
  // if the next task has the same priority as the task we're about
  // to run. This ensures the CPU is shared between all the
  // highest priority tasks.
  if (!pqueue_empty(&ready_tasks) && task_from_wait_node(pqueue_peek(&ready_tasks))->priority == next_task->priority)
  {
    if (next_task == running_task && !systick_fired)
      task_ticks = MIN(TIME_SLICE_TICKS, systick_val);
    else
      task_ticks = TIME_SLICE_TICKS;
  }
  running_task = next_task;

  // We need to check the sleeping tasks to see if one could wake us up early.
  struct list_head * node;
  list_for_each(node, &sleeping_tasks)
  {
    struct task * t = container_of(node, struct task, sleep_node);
    if (t->priority > next_task->priority)
    {
      // A higher priority sleeping task can reduce the ticks
      // to less than the time slice.
      task_ticks = MIN(task_ticks, t->sleep);
    }
    else if (t->priority == next_task->priority)
    {
      // An equal priority sleeping task can reduce the ticks
      // but still needs to respect the time slice or leftover time slice.
      task_ticks = MIN(task_ticks, MAX(TIME_SLICE_TICKS, t->sleep));
    }
  }

  // Setup the SysTick timer.
  // We also clear the IRQ if it fired while we were
  // handling the system call.
  SysTick->CTRL = SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
  SysTick->LOAD = task_ticks;
  SysTick->VAL = 0;
  SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk;
  __DSB();
  __ISB();
}

void systick_handle(void)
{
  svc_handle_yield();
  schedule();
}

void svc_handle(uint8_t value)
{
  switch (value)
  {
  case SYSCALL_YIELD:
    svc_handle_yield();
    break;
  case SYSCALL_SLEEP:
    svc_handle_sleep();
    break;
  case SYSCALL_MUTEX_LOCK:
    svc_handle_mutex_lock();
    break;
  case SYSCALL_MUTEX_UNLOCK:
    svc_handle_mutex_unlock();
    break;
  case SYSCALL_CHANNEL_SEND:
    svc_handle_channel_send();
    break;
  case SYSCALL_CHANNEL_RECV:
    svc_handle_channel_recv();
    break;
  case SYSCALL_CHANNEL_REPLY:
    svc_handle_channel_reply();
    break;
  case SYSCALL_TASK_RETURN:
    svc_handle_task_return();
    break;
  case SYSCALL_TASK_WAIT:
    svc_handle_task_wait();
    break;
  default:
    assert(false);
  }
  schedule();
  return;
}

void svc_handle_yield(void)
{
  // There's nothing special to do here. One of the following occured:
  // 1. The time slice expired.
  // 2. A task yielded the remainder of its time slice.
  // 3. The systick expired but no tasks were running.
  // Either way, the active task is no longer running.
  running_task->state = STATE_READY;
  task_wait_on(running_task, &ready_tasks);
}

void svc_handle_sleep(void)
{
  running_task->state = STATE_SLEEP;
  running_task->sleep *= SYSTICK_RELOAD_MS;
  list_push_back(&sleeping_tasks, &running_task->sleep_node);
}

void svc_handle_mutex_lock(void)
{
  struct mutex * mutex = running_task->mutex;
  assert(mutex != NULL);

  // Add the active task to the queue of tasks waiting for the mutex.
  running_task->state = STATE_MUTEX;
  task_wait_on(running_task, &mutex->waiting_tasks);

  // The owner of the mutex is now blocking whoever tried to lock it.
  task_add_blocked(mutex->owner, running_task);

  // Check if the task should timeout while waiting for the mutex.
  if (running_task->sleep > 0)
  {
    running_task->sleep *= SYSTICK_RELOAD_MS;
    list_push_back(&sleeping_tasks, &running_task->sleep_node);
  }
}

void svc_handle_mutex_unlock(void)
{
  struct mutex * mutex = running_task->mutex;
  assert(mutex != NULL);
  assert(!pqueue_empty(&mutex->waiting_tasks));

  // Determine who the new owner will be.
  struct task * new_owner = task_from_wait_node(pqueue_peek(&mutex->waiting_tasks));
  task_stop_waiting(new_owner);

  list_remove(&new_owner->sleep_node); // Stop sleeping in case of mutex_timed_lock().
  new_owner->mutex_locked = true;

  // Boths tasks are ready after the mutex is unlocked.
  // The running task is scheduled first so that it can finish its time slice.
  running_task->state = STATE_READY;
  new_owner->state = STATE_READY;
  task_wait_on(running_task, &ready_tasks);
  task_wait_on(new_owner, &ready_tasks);

  // The previous owner (running task) of the mutex is no longer blocking tasks waiting
  // for the mutex. The new owner of the mutex is now blocking all those tasks.
  mutex->owner = new_owner;
  task_remove_blocked(running_task, new_owner);

  struct list_head * node;
  pqueue_for_each(node, &mutex->waiting_tasks)
  {
    struct task * waiter = task_from_wait_node(node);
    task_remove_blocked(running_task, waiter);
    task_add_blocked(new_owner, waiter);
  }
}

void svc_handle_channel_send(void)
{
  // Save the message and channel
  struct channel * channel = running_task->channel;

  struct task * recv = channel->receive;
  if (recv != NULL)
  {
    // There's a task waiting for a message.
    recv->state = STATE_READY;
    task_wait_on(recv, &ready_tasks);
    channel->receive = NULL;
    channel->server = recv;

    // The sending task becomes reply blocked.
    running_task->state = STATE_CHANNEL_RPLY;
    channel->reply = running_task;

    // Copy the message from the sender to the receiver.
    assert(recv->channel_len >= running_task->channel_len);
    memcpy(recv->channel_msg, running_task->channel_msg, running_task->channel_len);
    recv->channel_len = running_task->channel_len;

    // The receiver is now blocking the sender;
    task_add_blocked(recv, running_task);
  }
  else
  {
    // No task is waiting for a message.
    running_task->state = STATE_CHANNEL_SEND;
    //task_wait_on(running_task, &channel->waiting_tasks);
  }
}

void svc_handle_channel_recv(void)
{
  struct task * send = get_next_task(&running_task->channel->waiting_tasks);
  if (send != NULL)
  {
    // A task has already sent a message to this channel.
    running_task->state = STATE_READY;
    task_wait_on(running_task, &ready_tasks);

    // The task that sent us a message is no longer waiting to send us a message.
    task_stop_waiting(send);

    // The task that sent the message becomes reply blocked.
    running_task->channel->reply = send;
    running_task->channel->server = running_task;
    send->state = STATE_CHANNEL_RPLY;

    // Copy the message from the sender to the receiver.
    assert(running_task->channel_len >= send->channel_len);
    memcpy(running_task->channel_msg, send->channel_msg, send->channel_len);
    running_task->channel_len = send->channel_len;

    // The receiver is now blocking the sender.
    task_add_blocked(running_task, send);
  }
  else
  {
    // No one has sent us a message :-(
    // We become receive blocked.
    running_task->state = STATE_CHANNEL_RECV;
    running_task->channel->receive = running_task;
  }
}

void svc_handle_channel_reply(void)
{
  struct channel * channel = running_task->channel;
  void * msg = running_task->channel_msg;
  size_t len = running_task->channel_len;

  // Copy the reply to the task that sent us a message.
  struct task * reply_task = channel->reply;
  assert(reply_task != NULL);
  assert(reply_task->channel_len >= len);
  memcpy(reply_task->channel_reply, msg, len);
  *reply_task->channel_reply_len = len;
  channel->reply = NULL;
  channel->server = NULL;

  // The task we replied to becomes unblocked.
  reply_task->state = STATE_READY;
  task_wait_on(reply_task, &ready_tasks);

  // The task that replied is still ready.
  running_task->state = STATE_READY;
  task_wait_on(running_task, &ready_tasks);

  // The task that replied is now longer blocking the sender.
  task_remove_blocked(running_task, reply_task);
}

void svc_handle_task_return(void)
{
  // When a task returns there should be exactly 0 or 1 tasks blocked on it.
  // If there's a task blocked on this task then it should be in the wait state.
  // It makes no sense for a task to return while holding a mutex or doing
  // something else that would cause a another task to block on it.
  // If no tasks are blocked on this task then it will go in the zombie state
  // until some other task retrieves the return code.
  assert(pqueue_empty(&running_task->blocking) ||
         pqueue_size(&running_task->blocking) == 1 &&
         container_of(pqueue_peek(&running_task->blocking), struct task, blocking)->state == STATE_WAIT);

  struct task * parent = container_of(running_task->family.parent, struct task, family);

  // Check if our parent is waiting for us or any of it's children.
  if (parent->state == STATE_WAIT &&
      (parent->wait == NULL || *parent->wait == NULL || *parent->wait == running_task))
  {
    // The parent is already waiting for us. Give it the return code.
    parent->wait_result = running_task->wait_result;

    if (parent->wait != NULL && *parent->wait == running_task)
    {
      // The parent task was waiting for us. We're no longer blocking it.
      task_remove_blocked(running_task, parent);
    }

    if (parent->wait != NULL)
    {
      *parent->wait = running_task;
    }

    // The parent task becomes ready and the returned task ceases to exist.
    parent->state = STATE_READY;
    task_wait_on(parent, &ready_tasks);
    task_destroy(running_task);
  }
  else
  {
    // The parent isn't waiting for us.
    // Go into the zombie state until the parent reaps us.
    running_task->state = STATE_ZOMBIE;
  }
}

void svc_handle_task_wait(void)
{
  // It makes no sense to call wait when we have no children.
  assert(tree_num_children(&running_task->family) > 0);

  if (running_task->wait != NULL && *running_task->wait != NULL)
  {
    // We're waiting for a specific child task.
    struct task * child = *running_task->wait;
    assert(child->family.parent == &running_task->family);
    if (child->state == STATE_ZOMBIE)
    {
      // The child task has already returned.
      // Take the return value and destroy it.
      running_task->state = STATE_READY;
      task_wait_on(running_task, &ready_tasks);
      running_task->wait_result = child->wait_result;
      task_destroy(child);
    }
    else
    {
      // The child task hasn't returned yet. Wait for it.
      task_add_blocked(child, running_task);
      running_task->state = STATE_WAIT;
    }
  }
  else
  {
    // We're waiting for any child task to return.
    // Check if there's already one that's returned.
    running_task->state = STATE_WAIT;
    struct tree_head * node;
    tree_for_each_direct(node, &running_task->family)
    {
      struct task * child = container_of(node, struct task, family);
      if (child->state == STATE_ZOMBIE)
      {
        // A child already terminated.
        if (running_task->wait != NULL)
        {
          *running_task->wait = child;
        }

        running_task->state = STATE_READY;
        task_wait_on(running_task, &ready_tasks);
        running_task->wait_result = child->wait_result;
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
