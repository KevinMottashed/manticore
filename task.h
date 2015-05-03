#ifndef TASK_H
#define TASK_H

#include "system.h"

#include <stdint.h>

#define MIN_PRIORITY (0)
#define MAX_PRIORITY (255)

typedef enum task_state_e
{
  STATE_RUNNING,
  STATE_READY,
  STATE_SLEEP,
} task_state_t;

// The is the context that will be saved and restored during context
// switches. The stack grows towards 0 which is why the registers
// saved by hardware appear in reverse order.
typedef struct context_s
{
  // The following registers are saved by software.
  uint32_t R8;
  uint32_t R9;
  uint32_t R10;
  uint32_t R11;
  uint32_t R4;
  uint32_t R5;
  uint32_t R6;
  uint32_t R7;
  
  // The following registers are saved by hardware when the ISR is entered.
  uint32_t R0;
  uint32_t R1;
  uint32_t R2;
  uint32_t R3;
  uint32_t R12;
  uint32_t LR;
  uint32_t PC;
  xPSR_Type xPSR;
} context_t;

typedef struct task_s
{
  uint32_t stackPointer;
  task_state_t state;
  unsigned int sleep;
  uint8_t priority;
  
  // 256 bytes of stack for tasks.
  // uint64_t is used to ensure 8-byte alignment.
  uint64_t stack[256/sizeof(uint64_t)];
} task_t;

#endif