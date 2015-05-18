/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <kevinmottashed@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return. 
 * -Kevin Mottashed
 * ----------------------------------------------------------------------------
 */

/*
 * See section B2.5 of the ARM architecture reference manual.
 * Updates to the SCS registers require the use of DSB/ISB instructions.
 * The SysTick and SCB are part of the SCS (System control space).
 */

#include "mutex.h"

#include "system.h"
#include "syscall.h"
#include "utils.h"

void mutex_init(mutex_t * mutex)
{
  static uint8_t mutexIdCounter = 0;
  mutex->id = mutexIdCounter++;
  mutex->locked = false;
  pqueue_init(&mutex->queue);
}

void mutex_lock(mutex_t * mutex)
{
  // Disable the system tick ISR
  SysTick->CTRL = SysTick_CTRL_ENABLE_Msk;
  __DSB();
  __ISB();

  if (!mutex->locked)
  {
    // The mutex is unlocked. Just take it.
    mutex->locked = true;
    
    // Reenable the systick ISR.
    SysTick->CTRL = SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
    __DSB();
    __ISB();
    if (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)
    {
      // Trigger the systick ISR if the timer expired
      // while we were locking the mutex.
      SCB->ICSR = SCB_ICSR_PENDSTSET_Msk;
      __DSB();
      __ISB();
    }
  }
  else
  {
    // The mutex is locked. Let the OS schedule the next task.
    syscallContext.mutex = mutex;
    asm ("SVC #3");
  }
}

bool mutex_trylock(mutex_t * mutex)
{
  // Disable the system tick ISR
  SysTick->CTRL = SysTick_CTRL_ENABLE_Msk;
  __DSB();
  __ISB();
  
  if (mutex->locked)
  {
    SysTick->CTRL = SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
    __DSB();
    __ISB();
    if (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)
    {
      SCB->ICSR = SCB_ICSR_PENDSTSET_Msk;
      __DSB();
      __ISB();
    }
    return false;
  }
  else
  {
    mutex->locked = true;
    SysTick->CTRL = SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
    __DSB();
    __ISB();
    if (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)
    {
      SCB->ICSR = SCB_ICSR_PENDSTSET_Msk;
      __DSB();
      __ISB();
    }
    return true;
  }
}

void mutex_unlock(mutex_t * mutex)
{
  // Disable the system tick ISR
  SysTick->CTRL = SysTick_CTRL_ENABLE_Msk;
  __DSB();
  __ISB();

  if (!pqueue_empty(&mutex->queue))
  {
    // Another task is waiting for this mutex.
    // Let the kernel run to unblock it.
    syscallContext.mutex = mutex;
    asm ("SVC #4");
  }
  else
  {
    // No one is waiting for this mutex.
    mutex->locked = false;
    SysTick->CTRL = SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
    __DSB();
    __ISB();
    if (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)
    {
      SCB->ICSR = SCB_ICSR_PENDSTSET_Msk;
      __DSB();
      __ISB();
    }
  }
}
