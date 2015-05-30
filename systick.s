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
  IMPORT isr_exit_to_kernel
  
  SECTION .data : CONST(2)
  PUBLIC  SysTickCtrlAddr
  DATA
SysTickCtrlAddr
  DC32 0xE000E010
    
  SECTION .text : CODE (2)
  THUMB

ClearSysTick:
  ; This function reads the systick control register
  ; to clear the COUNTFLAG field.
  ; R0 = SysTick->CTRL address
  ; R1 = SysTick->CTRL value
  LDR R0, =SysTickCtrlAddr
  LDR R0, [R0]
  LDR R1, [R0]
  BX LR

SysTick_Handler:
  ; The systick handler shouldn't run while in kernel mode.
  ; We can get into this situation when all tasks are sleeping
  ; and the kernel is waiting for this interrupt to wake it up.
  ; We could disable interrupts completly but then the user wouldn't
  ; be able to run non-OS ISRs while all the tasks are sleeping.
  LDR R0, =isr_exit_to_kernel
  LDR R0, [R0]
  MOV R1, LR
  CMP R0, R1
  BNE run_isr
  BX LR

run_isr:
  ; The systick handler only needs to save to save the current context
  ; and then let the kernel take over. Clear the COUNTFLAG field.
  BL SaveContext
  BL ClearSysTick
  B RestoreKernel
  
  END