/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <kevinmottashed@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return. 
 * -Kevin Mottashed
 * ----------------------------------------------------------------------------
 */

#include "gpio.h"

#include "system.h"

void gpio_init(void)
{
  // Enable GPIOC
  RCC_AHBENR_bit.IOPCEN = 1;
  
  // Make pins PC8 and PC9 output pins.
  GPIOC_MODER_bit.MODER8 = 1;
  GPIOC_MODER_bit.MODER9 = 1;
  
  // The LED pins are low by default and in low speed mode.
  // The internal pull-up and pull-down resistors are disabled.
}

void gpio_led3_on(void)
{
  // Use the atomic Set/Reset register to avoid race conditions.
  GPIOC_BSRR_bit.BS9 = 1;
}

void gpio_led3_off(void)
{
  GPIOC_BSRR_bit.BR9 = 1;
}

void gpio_led4_on(void)
{
  GPIOC_BSRR_bit.BS8 = 1;
}

void gpio_led4_off(void)
{
  GPIOC_BSRR_bit.BR8 = 1;
}
