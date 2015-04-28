#ifndef SYSCALL_H
#define SYSCALL_H

#define SYSCALL_NONE    (0) // No syscall was made
#define SYSCALL_YIELD   (1) // Task wishes to yield to another

void yield(void);

#endif