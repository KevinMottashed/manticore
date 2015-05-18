/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <kevinmottashed@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return. 
 * -Kevin Mottashed
 * ----------------------------------------------------------------------------
 */

#ifndef CLOCK_H
#define CLOCK_H

// The 8MHz HSE clock will be multiplied by 4 and used as the system clock.
#define SYSTEM_CLOCK (32000000)

void clock_init(void);

#endif
