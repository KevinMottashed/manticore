/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <kevinmottashed@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * -Kevin Mottashed
 * ----------------------------------------------------------------------------
 */

  NAME vector

  SECTION CSTACK : DATA : NOROOT (3)

  ; Handlers are defined as week so that they can be overloaded
  PUBWEAK Reset_Handler
  PUBWEAK NMI_Handler
  PUBWEAK HardFault_Handler
  PUBWEAK SVCall_Handler
  PUBWEAK PendSV_Handler
  PUBWEAK SysTick_Handler

  EXTERN __iar_program_start

  ; See section 12.1.3 of the STM32F0 reference manual
  SECTION .intvec : CONST(2)
  PUBLIC  __vector_table
  DATA
__vector_table
  DC32 SFE(CSTACK)
  ; Cortex-M0 IRQs
  DC32 Reset_Handler
  DC32 NMI_Handler
  DC32 HardFault_Handler
  DC32 0xDEADBEEF ; 0x10 - Reserved
  DC32 0xDEADBEEF ; 0x14 - Reserved
  DC32 0xDEADBEEF ; 0x18 - Reserved
  DC32 0xDEADBEEF ; 0x1C - Reserved
  DC32 0xDEADBEEF ; 0x20 - Reserved
  DC32 0xDEADBEEF ; 0x24 - Reserved
  DC32 0xDEADBEEF ; 0x28 - Reserved
  DC32 SVCall_Handler
  DC32 0xDEADBEEF ; 0x30 - Reserved
  DC32 0xDEADBEEF ; 0x34 - Reserved
  DC32 PendSV_Handler
  DC32 SysTick_Handler
  ; STM32-F0 IRQs
  ; TODO

  ; Default ISRs. Infinite loops.
  SECTION .text : CODE : NOROOT (2)
  THUMB
Reset_Handler:
  B __iar_program_start
NMI_Handler:
  B .
HardFault_Handler:
  B .
SVCall_Handler:
  B .
PendSV_Handler:
  B .
SysTick_Handler:
  B .

  END