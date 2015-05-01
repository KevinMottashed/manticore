  NAME context
  
  PUBLIC SaveContext
  PUBLIC ContextSwitch
  PUBLIC RestoreKernel
  PUBLIC isr_exit_to_kernel
    
  IMPORT savedContext
  IMPORT kernelContext
  
  SECTION .data : DATA (2)
  DATA
isr_exit_to_kernel
  DC32 0xFFFFFFF9
  
  SECTION .text : CODE (2)
  THUMB
  
  ; See exception entry in m0 user manual
#define STACK_R0_OFFSET 0x0
#define STACK_R1_OFFSET 0x4
#define STACK_R2_OFFSET 0x8
#define STACK_R3_OFFSET 0xc
#define STACK_R12_OFFSET 0x10
#define STACK_LR_OFFSET 0x14
#define STACK_PC_OFFSET 0x18
#define STACK_xPSR_OFFSET 0x1c

; Warning: This should be kept in sync with context_t
#define SAVED_R0_OFFSET 0x0
#define SAVED_R1_OFFSET 0x4
#define SAVED_R2_OFFSET 0x8
#define SAVED_R3_OFFSET 0xc
#define SAVED_R4_OFFSET 0x10
#define SAVED_R5_OFFSET 0x14
#define SAVED_R6_OFFSET 0x18
#define SAVED_R7_OFFSET 0x1c
#define SAVED_R8_OFFSET 0x20
#define SAVED_R9_OFFSET 0x24
#define SAVED_R10_OFFSET 0x28
#define SAVED_R11_OFFSET 0x2c
#define SAVED_R12_OFFSET 0x30
#define SAVED_SP_OFFSET 0x34
#define SAVED_LR_OFFSET 0x38
#define SAVED_PC_OFFSET 0x3c
#define SAVED_xPSR_OFFSET 0x40

; void SaveContext(void)
SaveContext:
  ; R0 = Stack pointer on exception entry
  ; R1 = Memory where the context will be saved
  ; R2 = Work area
  PUSH {R0,R1,R2,LR}

  MRS R0, PSP
  LDR R1, =savedContext
  LDR R1, [R1]
  
  ; Save all the registers stored on the stack
  ; Save R0 to R3
  LDR R2, [R0,#STACK_R0_OFFSET]
  STR R2, [R1,#SAVED_R0_OFFSET]
  LDR R2, [R0,#STACK_R1_OFFSET]
  STR R2, [R1,#SAVED_R1_OFFSET]
  LDR R2, [R0,#STACK_R2_OFFSET]
  STR R2, [R1,#SAVED_R2_OFFSET]
  LDR R2, [R0,#STACK_R3_OFFSET]
  STR R2, [R1,#SAVED_R3_OFFSET]
  ; Save R12
  LDR R2, [R0,#STACK_R12_OFFSET]
  STR R2, [R1,#SAVED_R12_OFFSET]
  ; Save LR
  LDR R2, [R0,#STACK_LR_OFFSET]
  STR R2, [R1,#SAVED_LR_OFFSET]
  ; Save PC
  LDR R2, [R0,#STACK_PC_OFFSET]
  STR R2, [R1,#SAVED_PC_OFFSET]
  ; Save xPSR
  LDR R2, [R0,#STACK_xPSR_OFFSET]
  STR R2, [R1,#SAVED_xPSR_OFFSET]
  
  ; Save R4 to R7
  STR R4, [R1,#SAVED_R4_OFFSET]
  STR R5, [R1,#SAVED_R5_OFFSET]
  STR R6, [R1,#SAVED_R6_OFFSET]
  STR R7, [R1,#SAVED_R7_OFFSET]
  
  ; Save R8 to R11
  MOV R2, R8
  STR R2, [R1,#SAVED_R8_OFFSET]
  MOV R2, R9
  STR R2, [R1,#SAVED_R9_OFFSET]
  MOV R2, R10
  STR R2, [R1,#SAVED_R10_OFFSET]
  MOV R2, R11
  STR R2, [R1,#SAVED_R11_OFFSET]
  
  ; Save SP. Add 0x20 to discard the expection entry stack frame.
  MOVS R2, #0x20
  ADD R0, R0, R2
  STR R0, [R1,#SAVED_SP_OFFSET]

  POP {R0,R1,R2,PC}
  
; Switch from the kernel to a task
; void ContextSwitch(volatile context_t * context)
; NO RETURN
ContextSwitch:
  ; R0 = Address to restore the task context from
  ; R1 = Address to save the kernel context
  ; R2 = Work area
  PUSH {R0, R1}
  LDR R1, =kernelContext
  
  ; Save kernel context
  ; Save R2 - R7
  STR R2, [R1,#SAVED_R2_OFFSET]
  STR R3, [R1,#SAVED_R3_OFFSET]
  STR R4, [R1,#SAVED_R4_OFFSET]
  STR R5, [R1,#SAVED_R5_OFFSET]
  STR R6, [R1,#SAVED_R6_OFFSET]
  STR R7, [R1,#SAVED_R7_OFFSET]
  
  ; Save R0 - R1
  POP {R2, R3}
  STR R2, [R1,#SAVED_R0_OFFSET]
  STR R3, [R1,#SAVED_R1_OFFSET]
  
  ; Save R8-R12
  MOV R2, R8
  STR R2, [R1,#SAVED_R8_OFFSET]
  MOV R2, R9
  STR R2, [R1,#SAVED_R9_OFFSET]
  MOV R2, R10
  STR R2, [R1,#SAVED_R10_OFFSET]  
  MOV R2, R11
  STR R2, [R1,#SAVED_R11_OFFSET]
  MOV R2, R12
  STR R2, [R1,#SAVED_R12_OFFSET]
  
  ; The kernel has it's own SP so it doesn't need to be saved.
  ; The LR register doesn't need to be saved because it will be on the stack.
  
  ; Save the next PC after this function is called (LR)
  MOV R2, LR
  STR R2, [R1,#SAVED_PC_OFFSET]
  
  ; Save xPSR
  MRS R2, PSR
  STR R2, [R1,#SAVED_xPSR_OFFSET]
  
  ; Restore task context 
  ; Restore R3 - R7
  LDR R3, [R0,#SAVED_R3_OFFSET]
  LDR R4, [R0,#SAVED_R4_OFFSET]
  LDR R5, [R0,#SAVED_R5_OFFSET]
  LDR R6, [R0,#SAVED_R6_OFFSET]
  LDR R7, [R0,#SAVED_R7_OFFSET]
  
  ; Restore R8 - R12
  LDR R2, [R0,#SAVED_R8_OFFSET]
  MOV R8, R2
  LDR R2, [R0,#SAVED_R9_OFFSET]
  MOV R9, R2  
  LDR R2, [R0,#SAVED_R10_OFFSET]
  MOV R10, R2
  LDR R2, [R0,#SAVED_R11_OFFSET]
  MOV R11, R2
  LDR R2, [R0,#SAVED_R12_OFFSET]
  MOV R12, R2
  
  ; Restore LR
  LDR R2, [R0,#SAVED_LR_OFFSET]
  MOV LR, R2
  
  ; Restore PSP
  LDR R2, [R0,#SAVED_SP_OFFSET]
  MSR PSP, R2
  
  ; Restore xPSR
  LDR R2, [R0,#SAVED_xPSR_OFFSET]
  MSR PSR, R2
  
  ; Switch to the process stack
  MOVS R2, #0x2
  MSR CONTROL, R2
  
  ; Restore R0 - R2 and PC
  LDR R2, [R0,#SAVED_PC_OFFSET]
  MOVS R1, #1
  ORRS R2, R2, R1 ; Need to set bit 0 of PC for THUMB mode
  PUSH {R2}
  LDR R2, [R0,#SAVED_R2_OFFSET]
  PUSH {R2}  
  LDR R2, [R0,#SAVED_R1_OFFSET]
  PUSH {R2}
  LDR R2, [R0,#SAVED_R0_OFFSET]
  PUSH {R2}
  POP {R0, R1, R2, PC}

; Change context from an ISR handler back to the kernel.
; void RestoreKernel(void)
; NO RETURN
RestoreKernel:
  ; R0 = area to restore the kernel context from
  ; R1 - R2 = work area
  LDR R0, =kernelContext
  
  ; Restore xPSR
  LDR R1, [R0,#SAVED_xPSR_OFFSET]
  MOVS R2, #1
  LSLS R2, R2, #24
  ORRS R1, R1, R2
  PUSH {R1}
  
  ; Restore PC
  LDR R1, [R0,#SAVED_PC_OFFSET]
  PUSH {R1}
  
  ; Restore a dummy LR value
  MOVS R1, #0
  PUSH {R1}
  
  ; Restore R12
  LDR R1, [R0,#SAVED_R12_OFFSET]
  PUSH {R1}
  
  ; Restore R3 - R0
  LDR R1, [R0,#SAVED_R3_OFFSET]
  PUSH {R1}
  LDR R1, [R0,#SAVED_R2_OFFSET]
  PUSH {R1}
  LDR R1, [R0,#SAVED_R1_OFFSET]
  PUSH {R1}  
  LDR R1, [R0,#SAVED_R0_OFFSET]
  PUSH {R1}
  
  ; Restore R4 - R7
  LDR R4, [R0,#SAVED_R4_OFFSET]
  LDR R5, [R0,#SAVED_R5_OFFSET]
  LDR R6, [R0,#SAVED_R6_OFFSET]
  LDR R7, [R0,#SAVED_R7_OFFSET]
  
  ; Restore R8 - R11
  LDR R1, [R0,#SAVED_R8_OFFSET]
  MOV R8, R1
  LDR R1, [R0,#SAVED_R9_OFFSET]
  MOV R9, R1
  LDR R1, [R0,#SAVED_R10_OFFSET]
  MOV R10, R1
  LDR R1, [R0,#SAVED_R11_OFFSET]  
  MOV R11, R1
  
  LDR R1, =isr_exit_to_kernel
  LDR R1, [R1]
  BX R1
  
  END