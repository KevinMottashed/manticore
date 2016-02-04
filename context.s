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
  PUBLIC RestoreKernel
  PUBLIC PendSV_Handler
  PUBLIC isr_exit_to_kernel

  IMPORT savedStackPointer
  IMPORT kernelContext
  IMPORT SysTickCtrlAddr

  SECTION .data : DATA (2)
  DATA
isr_exit_to_kernel
  DC32 0xFFFFFFF9
isr_exit_to_task
  DC32 0xFFFFFFFD

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
  LDR R1, =savedStackPointer
  LDR R1, [R1]
  STR R0, [R1]

  BX LR

; Change context from an ISR handler back to the kernel.
; void RestoreKernel(void)
; NO RETURN
RestoreKernel:
  ; Restore R8 - R11
  POP {R0 - R3}
  MOV R8, R0
  MOV R9, R1
  MOV R10, R2
  MOV R11, R3

  ; Restore R4 - R7
  POP {R4 - R7}

  LDR R0, =isr_exit_to_kernel
  LDR R0, [R0]
  BX R0

PendSV_Handler:
  ; Save the kernel context R4 - R11.
  ; We don't need to save the stack pointer because the kernel uses its own.
  ; Save R4 - R7
  PUSH {R4 - R7}

  ; Save R8 - R11
  MOV R0, R8
  MOV R1, R9
  MOV R2, R10
  MOV R3, R11
  PUSH {R0 - R3}

  ; Restore the task context.
  ; Load the task SP.
  LDR R1, =savedStackPointer
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

  ; Substract 32 bytes from the task SP after restoring R4 - R11.
  MSR PSP, R0

  ; Enable the systick IRQ
  LDR R0, =SysTickCtrlAddr
  LDR R0, [R0]
  MOVS R1, #3
  STR R1, [R0]

  ; Return to the task context.
  LDR R0, =isr_exit_to_task
  LDR R0, [R0]
  BX R0

  END