/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <kevinmottashed@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * -Kevin Mottashed
 * ----------------------------------------------------------------------------
 */

  NAME context

  PUBLIC SaveContext
  PUBLIC RestoreContext

  IMPORT running_task

  SECTION .text : CODE (2)
  THUMB

; void SaveContext(void)
SaveContext:
  ; Save R4 to R7.
  ; The following is equivalent to PUSH {R4-R7} on the process stack.
  MRS R0, PSP
  MOVS R1, #16
  SUBS R0, R0, R1
  STM R0!, {R4-R7}

  ; Save R8 to R11
  ; The following is equivalent to PUSH {R8-R11} on the process stack.
  MOVS R1, #32
  SUBS R0, R0, R1
  MOV R4, R8
  MOV R5, R9
  MOV R6, R10
  MOV R7, R11
  STM R0!, {R4-R7}

  ; Save SP.
  ; The stack pointer includes the equivalent PUSH instructions above.
  MOVS R1, #16
  SUBS R0, R0, R1
  LDR R1, =running_task
  LDR R1, [R1]
  STR R0, [R1]
  BX LR

; void RestoreContext(void)
RestoreContext:
  ; Read the stack pointer
  LDR R1, =running_task
  LDR R1, [R1]
  LDR R0, [R1]

  ; Restore R8 - R11
  ; The following is equivalent to POP {R8-R11} on the process stack.
  LDM R0!, {R4-R7}
  MOV R8, R4
  MOV R9, R5
  MOV R10, R6
  MOV R11, R7

  ; Restore R4 - R7
  ; The following is equivalent to POP {R4-R7} on the process stack.
  LDM R0!, {R4-R7}

  ; Set the process stack pointer.
  MSR PSP, R0
  BX LR

  END