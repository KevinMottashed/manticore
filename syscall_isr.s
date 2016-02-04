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
  PUBLIC DisableSysTickIrq

  IMPORT SaveContext
  IMPORT RestoreKernel
  IMPORT syscallValue
  IMPORT SysTickCtrlAddr

  SECTION .data : CONST (2)
PendStClear
  DC32 0x02000000

ICSR ; Interrupt control and status register
  DC32 0xE000ED04

  SECTION .text : CODE (2)
  THUMB

DisableSysTickIrq:
  ; Disable the systick IRQ but leave the systick ticking.
  ; The COUNTFLAG isn't touched.
  LDR R0, =SysTickCtrlAddr
  LDR R0, [R0]
  MOVS R1, #1
  STR R1, [R0]
  BX LR

ClearSysTickIrq:
  ; Clear a pending systick interrupt
  LDR R0, =ICSR
  LDR R0, [R0]
  LDR R1, =PendStClear
  LDR R1, [R1]
  STR R1, [R0]
  BX LR

; Saves the value associated with the SVC instruction.
; void SaveSVC(void)
SaveSVC:
  ; R0 - R1 = Work area
  MRS R0, PSP
  MOVS R1, #0x18
  ADD R0, R0, R1
  LDR R0, [R0] ; R0 now points to the instruction after the SVC call
  SUBS R0, R0, #2 ; Move back to the SVC instruction.
  LDRB R0, [R0] ; Read the LSB of the instruction which will be the SVC value.
  LDR R1, =syscallValue
  STRB R0, [R1]
  BX LR

SVCall_Handler:
  ; We need to save the task context, save the SVC value and then
  ; let the kernel handle the system call.
  ; The SysTick IRQ has the same priority as SVC so we don't
  ; need to worry about being preempted and losing the context.
  BL DisableSysTickIrq
  BL ClearSysTickIrq
  BL SaveContext
  BL SaveSVC
  B RestoreKernel

  END