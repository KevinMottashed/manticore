/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <kevinmottashed@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return. 
 * -Kevin Mottashed
 * ----------------------------------------------------------------------------
 */

#include "syscall.h"

#include "system.h"

#include <stdint.h>

syscall_context_t syscallContext;

void yield(void)
{
  // Disable the system tick ISR
  SysTick->CTRL = SysTick_CTRL_ENABLE_Msk;
  __DSB();
  __ISB();
  
  // Execute the system call
  asm("SVC #1");
}

void sleep(unsigned int seconds)
{
  // TODO: Test the max value. It won't work due to overflow so fix it.
  delay(seconds * 1000);
}

void delay(unsigned int ms)
{
  // Disable the system tick ISR
  SysTick->CTRL = SysTick_CTRL_ENABLE_Msk;
  __DSB();
  __ISB();
  
  syscallContext.sleep = ms;
  
  // Execute the system call
  asm("SVC #2");
}
