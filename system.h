#ifndef SYSTEM_H
#define SYSTEM_H

#define __NVIC_PRIO_BITS (2)

typedef enum IRQn
{
  NonMaskableInt_IRQn = -14,
  HardFault_IRQn = -13,
  SVCall_IRQn = -5,
  PendSV_IRQn = -2,
  SysTick_IRQn = -1,
  // TODO: STM32F0 IRQs
} IRQn_Type;

#include "core_cm0.h"
#include "iostm32f051x8.h"

#endif
