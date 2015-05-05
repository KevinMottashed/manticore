/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <kevinmottashed@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return. 
 * -Kevin Mottashed
 * ----------------------------------------------------------------------------
 */

#ifndef SYSCALL_H
#define SYSCALL_H

#define SYSCALL_NONE    (0) // No syscall was made
#define SYSCALL_YIELD   (1) // Task wishes to yield to another
#define SYSCALL_SLEEP   (2) // Sleep for x milli seconds

typedef union SyscallContext_u
{
  unsigned int sleep;
} SyscallContext_t;

extern SyscallContext_t syscallContext;

void yield(void);

void sleep(unsigned int seconds);
void delay(unsigned int ms);

#endif
