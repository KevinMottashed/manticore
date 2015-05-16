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

#include "task.h"
#include "system.h"
#include "syscall.h"
#include "utils.h"
#include "hardware.h"
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

static task_t * runningTask;
static pqueue_t readyQueue;
static vector_t sleepVector;

static void kernel_update_sleep(unsigned int ticks);
static void kernel_handle_syscall(uint8_t value);

void kernel_init(void)
{
  // Initialize the ready queue and the vector of sleeping tasks.
  pqueue_init(&readyQueue);
  vector_init(&sleepVector);
}

void kernel_main(void)
{
  // Enable SysTick IRQ.
  SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
  
  unsigned int kernelTicks = 0;
  unsigned int systickReload = 0xFFFFFF;
  
  while (true)
  {
    // The systick value when entering the kernel.
    int kernelStartTicks = SysTick->VAL;
    
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
    SysTick->LOAD = 0;
    
    if (nextTask != NULL)
    {
      // We have a task to schedule.
      runningTask = nextTask;
      runningTask->state = STATE_RUNNING;
      savedStackPointer = &runningTask->stackPointer;
      
      // Trigger a PendSV interrupt to return to task context.
      SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
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

void kernel_create_task(task t, void * arg, void * stack, uint32_t stackSize, uint8_t priority)
{
  assert(stackSize >= sizeof(context_t));
  
  context_t context;
  memset(&context, 0, sizeof(context)); // Most registers will start off as zero.
  context.R0 = (uint32_t)arg;
  context.PC = (uint32_t)t; // The program counter starts at the task entry point.
  context.xPSR.b.T = 1; // Enabled Thumb mode.
  
  task_t * task = heap_malloc(sizeof(*task));
  
  static uint8_t taskIdCounter = 0;
  task->id = taskIdCounter++;
  task->state = STATE_READY;
  task->priority = priority;
  task->stackPointer = (uint32_t)stack + stackSize;
  task->stackPointer &= ~0x7; // The stack must be 8 byte aligned.
  task->stackPointer -= sizeof(context_t);
  memcpy((void*)task->stackPointer, &context, sizeof(context));
  
  // Add the task to the queue of ready tasks.
  pqueue_push(&readyQueue, task, task->priority);
}

void kernel_update_sleep(unsigned int ticks)
{
  // Iterate through all the tasks to update the sleep time left.
  for (int i = 0; i < vector_size(&sleepVector);)
  {
    task_t * t = vector_at(&sleepVector, i);
    if (ticks >= t->sleep)
    {
      // The task is ready. Move it from the sleep vector
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
    break;
  case SYSCALL_SLEEP:
    runningTask->state = STATE_SLEEP;
    runningTask->sleep = syscallContext.sleep * SYSTICK_RELOAD_MS;
    vector_push_back(&sleepVector, runningTask);
    break;
  case SYSCALL_MUTEX_LOCK:
    {
      // Add the active task to the queue of tasks waiting for the mutex.
      mutex_t * mutex = syscallContext.mutex;      
      runningTask->state = STATE_MUTEX;
      pqueue_push(&mutex->queue, runningTask, runningTask->priority);
      break;
    }
  case SYSCALL_MUTEX_UNLOCK:
    {
      mutex_t * mutex = syscallContext.mutex;
      
      // Unblock the next highest priority task waiting for this mutex.
      task_t * next = pqueue_pop(&mutex->queue);
      next->state = STATE_READY;
      pqueue_push(&readyQueue, next, next->priority);
      
      // A task is still ready after unlocking a mutex.
      runningTask->state = STATE_READY;
      pqueue_push(&readyQueue, runningTask, runningTask->priority);      
    }
    break;
  default:
    assert(false);
  }  
}
