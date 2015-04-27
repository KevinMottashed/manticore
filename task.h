#ifndef TASK_H
#define TASK_H

#include <stdint.h>

// Warning: Member offsets are used in systick.s
typedef struct context_s
{
  uint32_t R[13]; // R0 - R12
  uint32_t SP;
  uint32_t LR;
  uint32_t PC;
  uint32_t xPSR;
} context_t;

typedef struct task_s
{
  context_t context;
  uint32_t stack[256/sizeof(uint32_t)]; // 256 bytes of stack for tasks
} task_t;

#endif