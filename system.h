/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <kevinmottashed@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return. 
 * -Kevin Mottashed
 * ----------------------------------------------------------------------------
 */

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
