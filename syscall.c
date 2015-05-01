#include "syscall.h"

#include "system.h"

#include <stdint.h>

SyscallContext_t syscallContext;

void yield(void)
{
  // Disable the system tick
  SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
  __DSB();
  
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
  // Disable the system tick
  SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
  __DSB();
  
  syscallContext.sleep = ms;
  
  // Execute the system call
  asm("SVC #2");
}
