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
  IMPORT systick_handle
  IMPORT RestoreContext

  SECTION .text : CODE (2)
  THUMB

SysTick_Handler:
  ; Save the context then handle the context switch.
  ; Restore the context of the next task and return to user land.
  PUSH {R0, LR}
  BL SaveContext
  BL systick_handle
  BL RestoreContext
  POP {R0, PC}

  END