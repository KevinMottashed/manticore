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
  IMPORT RestoreKernel
  IMPORT syscallValue
  
  SECTION .text : CODE (2)
  THUMB
  
; Saves the value associated with the SVC instruction.
; void SaveSVC(void)
SaveSVC:
  ; R0 - R1 = Work area
  MRS R0, PSP
  MOVS R1, #0x18
  ADD R0, R0, R1
  LDR R0, [R0] ; R0 now points to intruction after the SVC call
  SUBS R0, R0, #2 ; Move back to the SVC instruction.
  LDRB R0, [R0] ; Read the LSB of the instruction which will be the SVC value.
  LDR R1, =syscallValue
  STR R0, [R1]
  BX LR
  
SVCall_Handler:
  ; We need to save the task context, save the SVC value and then
  ; let the kernel handle the system call.
  BL SaveContext
  BL SaveSVC
  B RestoreKernel
  
  END