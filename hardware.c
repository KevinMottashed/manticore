#include "hardware.h"

#include "system.h"

void hardware_init(void)
{
  // For now we only need to initialize the clock
  // Turn on the external clock
  RCC_CR_bit.HSEON = 1;
  while (RCC_CR_bit.HSERDY != 1);
  
  // Configure the PLL
  RCC_CFGR_bit.PLLMUL = 2;
  RCC_CFGR_bit.PLLSRC = 1;
  
  // Turn on the PLL
  RCC_CR_bit.PLLON = 1;
  while (RCC_CR_bit.PLLRDY != 1);
  
  // Switch to using the PLL as the clock source
  RCC_CFGR_bit.SW = 2;
  while (RCC_CFGR_bit.SW != 2);
}
