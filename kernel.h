#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>

typedef void (*task)(void *);

void kernel_main(void);

void kernel_create_task(task t, uint8_t priority);

#endif
