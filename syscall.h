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
