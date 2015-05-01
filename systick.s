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

DisableSysTick:
  ; R0 = SysTick->CTRL address
  ; R1 = SysTick->CTRL value
  ; R2 = Work area
  LDR R0, =SysTickCtrlAddr
  LDR R0, [R0]
  LDR R1, [R0]
  MOVS R2, #1
  BICS R1, R1, R2
  STR R1, [R0]
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
  ; and then let the kernel take over. Disable the systick ISR and
  ; let the kernel reenable it. 
  BL SaveContext
  BL DisableSysTick
  B RestoreKernel
  
  END