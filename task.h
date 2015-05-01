#ifndef TASK_H
#define TASK_H

#include <stdint.h>

typedef enum task_state_e
{
  STATE_RUNNING,
  STATE_READY,
  STATE_SLEEP,
} task_state_t;

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
  task_state_t state;
  unsigned int sleep;
  
  // 256 bytes of stack for tasks.
  // uint64_t is used to ensure 8-byte alignment.
  uint64_t stack[256/sizeof(uint64_t)];
} task_t;

#endif