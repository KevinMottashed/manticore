  NAME systick
  
  PUBLIC SysTick_Handler
  
  IMPORT SaveContext
  IMPORT RestoreKernel
    
  SECTION .text : CODE (2)
  THUMB
  
SysTick_Handler:
  ; The systick handler only needs to save to save the current context
  ; and then let the kernel take over.
  BL SaveContext  
  B RestoreKernel  
  
  END