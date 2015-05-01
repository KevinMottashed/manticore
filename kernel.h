#ifndef KERNEL_H
#define KERNEL_H

typedef void (*task)(void *);

void kernel_main(void);

void kernel_create_task(task t);

#endif
