/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <kevinmottashed@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * -Kevin Mottashed
 * ----------------------------------------------------------------------------
 */

  NAME syscall

  PUBLIC SVCall_Handler

  IMPORT SaveContext
  IMPORT RestoreContext
  IMPORT svc_handle
  IMPORT running_task

  SECTION .data : CONST (2)
PendStClear
  DC32 0x02000000
ICSR ; Interrupt control and status register
  DC32 0xE000ED04
isr_exit_to_task
  DC32 0xFFFFFFFD

  SECTION .text : CODE (2)
  THUMB

; Returns the value associated with the SVC instruction.
; uint8_t GetSVC(void)
GetSVC:
  ; R0 - R1 = Work area
  MRS R0, PSP
  MOVS R1, #0x18
  ADD R0, R0, R1
  LDR R0, [R0] ; R0 now points to the instruction after the SVC call
  SUBS R0, R0, #2 ; Move back to the SVC instruction.
  LDRB R0, [R0] ; Read the LSB of the instruction which will be the SVC value.
  BX LR

SVCall_Handler:
  ; We need to save the task context, save the SVC value and then
  ; let handle the system call.
  ; The SysTick IRQ has the same priority as SVC so we don't
  ; need to worry about being preempted and losing the context.
  MOVS R0, #0
  LDR R1, =running_task
  LDR R1, [R1]
  CMP R1, #0
  BEQ no_context
  BL SaveContext
  BL GetSVC
no_context:
  BL svc_handle
  BL RestoreContext

  LDR R0, =isr_exit_to_task
  LDR R0, [R0]
  BX R0

  END