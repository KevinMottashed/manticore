#include "syscall.h"

#include "system.h"

#include <stdint.h>

void yield(void)
{
  // Disable the system tick
  SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
  __ISB();
  
  // Execute the system call
  asm("SVC #1");
}
