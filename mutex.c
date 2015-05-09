/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <kevinmottashed@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return. 
 * -Kevin Mottashed
 * ----------------------------------------------------------------------------
 */


#include "mutex.h"

#include "system.h"
#include "syscall.h"

void mutex_init(mutex_t * mutex)
{
  static uint8_t mutexIdCounter = 0;
  mutex->id = mutexIdCounter++;
  mutex->locked = false;
  mutex->blocked = 0;
}

void mutex_lock(mutex_t * mutex)
{
  // Disable the system tick ISR
  SysTick->CTRL = SysTick_CTRL_ENABLE_Msk;
  __DSB();

  if (!mutex->locked)
  {
    // The mutex is unlocked. Just take it.
    mutex->locked = true;
    
    // Reenable the systick ISR.
    SysTick->CTRL = SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
    if (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)
    {
      // Trigger the systick ISR if the timer expired
      // while we were locking the mutex.
      SCB->ICSR = SCB_ICSR_PENDSTSET_Msk;
    }
  }
  else
  {
    // The mutex is locked. Let the OS schedule the next task.
    mutex->blocked++;
    syscallContext.mutex = mutex;
    asm ("SVC #3");
  }
}

bool mutex_trylock(mutex_t * mutex)
{
  // Disable the system tick ISR
  SysTick->CTRL = SysTick_CTRL_ENABLE_Msk;
  __DSB();
  
  if (mutex->locked)
  {
    SysTick->CTRL = SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
    if (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)
    {
      SCB->ICSR = SCB_ICSR_PENDSTSET_Msk;
    }
    return false;
  }
  else
  {
    mutex->locked = true;
    SysTick->CTRL = SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
    if (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)
    {
      SCB->ICSR = SCB_ICSR_PENDSTSET_Msk;
    }
    return true;
  }
}

void mutex_unlock(mutex_t * mutex)
{
  // Disable the system tick ISR
  SysTick->CTRL = SysTick_CTRL_ENABLE_Msk;
  __DSB();

  if (mutex->blocked > 0)
  {
    // Another task is waiting for this mutex.
    // Let the kernel run to unblock it.
    mutex->blocked--;
    syscallContext.mutex = mutex;
    asm ("SVC #4");
  }
  else
  {
    // No one is waiting for this mutex.
    mutex->locked = false;
    SysTick->CTRL = SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
    if (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)
    {
      SCB->ICSR = SCB_ICSR_PENDSTSET_Msk;
    }
  }
}
