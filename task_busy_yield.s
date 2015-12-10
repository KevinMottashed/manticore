/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <kevinmottashed@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * -Kevin Mottashed
 * ----------------------------------------------------------------------------
 */

  NAME task_busy_yield

  PUBLIC Task_Busy_Yield

  // A bunch of magic numbers to load the registers with.
  SECTION .data : CONST(2)
  DATA
MagicR0
  DC32 0x01234567
MagicR1
  DC32 0x10101010
MagicR2
  DC32 0x20202020
MagicR3
  DC32 0x30303030
MagicR4
  DC32 0x40404040
MagicR5
  DC32 0x50505050
MagicR6
  DC32 0x60606060
MagicR7
  DC32 0x70707070
MagicR8
  DC32 0x80808080
MagicR9
  DC32 0x90909090
MagicR10
  DC32 0xA0A0A0A0
MagicR11
  DC32 0xB0B0B0B0
MagicR12
  DC32 0xC0C0C0C0
MaxUInt32
  DC32 0xFFFFFFFF
MaxInt32
  DC32 0x7FFFFFFF

  SECTION .text : CODE (2)
  THUMB

  // This task will set registers and do the yield() system call.
  // The registers will then be verified to make sure they still have
  // their original values. The Z, C, N and V flags will also be verified.
Task_Busy_Yield:
  // Save R4 - R11 and LR
  PUSH {R0, R4-R7, LR}
  MOV R0, R8
  MOV R1, R9
  MOV R2, R10
  MOV R3, R11
  PUSH {R0-R3}

  // Verify that R0-R4 are preserved
  LDR R4, =MagicR0
  LDM R4!, {R0-R3}

  SVC #1 // yield()

  LDR R4, =MagicR0
  LDM R4, {R4-R7}
  CMP R0, R4
  BNE fail
  CMP R1, R5
  BNE fail
  CMP R2, R6
  BNE fail
  CMP R3, R7
  BNE fail

  // Verify that R4-R7 are preserved
  LDR R0, =MagicR4
  LDM R0!, {R4-R7}

  SVC #1 // yield()

  LDR R0, =MagicR4
  LDM R0, {R0-R3}
  CMP R0, R4
  BNE fail
  CMP R1, R5
  BNE fail
  CMP R2, R6
  BNE fail
  CMP R3, R7
  BNE fail

  // Verify that R8-R12 are preserved
  LDR R0, =MagicR8
  LDM R0, {R0-R4}
  MOV R8, R0
  MOV R9, R1
  MOV R10, R2
  MOV R11, R3
  MOV R12, R4

  SVC #1 // yield()

  LDR R0, =MagicR8
  LDM R0, {R0-R4}
  CMP R0, R8
  BNE fail
  CMP R1, R9
  BNE fail
  CMP R2, R10
  BNE fail
  CMP R3, R11
  BNE fail
  CMP R4, R12
  BNE fail

  // Verify Z == 1 (equal) is preserved
  MOVS R0, #1
  MOVS R1, #1
  CMP R0, R1
  SVC #1 // yield()
  BNE fail

  // Verify Z == 0 (not equal) is preserved
  MOVS R0, #0
  MOVS R1, #1
  CMP R0, R1
  SVC #1 // yield()
  BEQ fail

  // Verify C == 1 (carry) is preserved
  LDR R0, =MaxUInt32
  LDR R0, [R0]
  MOVS R1, #1
  ADDS R0, R0, R1
  SVC #1 // yield()
  BCC fail

  // Verify C == 0 (no carry) is preserved
  MOVS R0, #0
  MOVS R1, #1
  ADDS R0, R0, R1
  SVC #1 // yield()
  BCS fail

  // Verify N == 1 (negative) is preserved
  MOVS R0, #0
  MOVS R1, #1
  SUBS R0, R0, R1
  SVC #1 // yield()
  BPL fail

  // Verify N == 0 (positive) is preserved
  MOVS R0, #1
  MOVS R1, #0
  SUBS R0, R0, R1
  SVC #1 // yield()
  BMI fail

  // Verify V == 1 (signed overflow) is preserved
  LDR R0, =MaxInt32
  LDR R0, [R0]
  MOVS R1, #1
  ADDS R0, R0, R1
  SVC #1 // yield()
  BVC fail

  // Verify V == 0 (no signed overflow) is preserved
  MOVS R0, #0
  MOVS R1, #1
  ADDS R0, R0, R1
  SVC #1 // yield()
  BVS fail

  // Restore R4-R11 and LR
  POP {R0-R3}
  MOV R8, R0
  MOV R9, R1
  MOV R10, R2
  MOV R11, R3
  POP {R0, R4-R7, PC}

fail:
  BKPT #0
  B fail

  END