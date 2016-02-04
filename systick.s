/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <kevinmottashed@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * -Kevin Mottashed
 * ----------------------------------------------------------------------------
 */

  NAME systick

  PUBLIC SysTick_Handler

  IMPORT SaveContext
  IMPORT RestoreKernel
  IMPORT DisableSysTickIrq
  IMPORT isr_exit_to_kernel

  SECTION .data : CONST(2)
  PUBLIC SysTickCtrlAddr
  DATA
SysTickCtrlAddr
  DC32 0xE000E010

  SECTION .text : CODE (2)
  THUMB

SysTick_Handler:
  ; The systick handler only needs to save the task context
  ; and then let the kernel take over.
  ; Sometimes there's no task context to save. This happens
  ; when the systick expires in the PendSV handler. This interrupt
  ; is tail chained from the PendSV handler so we never entered task context.
  ; Bit 3 in the LR register tells us if it's tail chained or not.
  MOVS R0, #8
  MOV R1, LR
  TST R0, R1
  BEQ no_context_save
  BL SaveContext
no_context_save:
  BL DisableSysTickIrq
  LDR R0, =RestoreKernel
  BX R0

  END