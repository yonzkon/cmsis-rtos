    .syntax unified
    .cpu cortex-m3
    .fpu softvfp
    .thumb

    .extern syscall_table

    .global __syscall
__syscall:
    STMFD R13!, {R1 - R7, R14}
    MOV R7, R0
    MOV R0, R1
    MOV R1, R2
    MOV R2, R3
    MOV R3, R4
    MOV R4, R5
    swi 0                         // to at_syscall
    LDMFD R13!, {R1 - R7, R15}

    .global at_syscall
at_syscall:
    STMFD R13!, {R6 - R8, R14}
    MRS R8, CONTROL
    AND R8, #0b10
    MSR CONTROL, R8

    LDR R8, =syscall_table // R8 = syscall_tabl
    LSL R7, #2             // R7 *= 4
    LDR R6, [R8, R7]       // R6 = syscall_tabl + R7
    CMP R6, #0
    IT NE
    BLXNE R6
    STR R0, [SP, #16]

    MRS R8, CONTROL
    ORR R8, #1
    MSR CONTROL, R8
    LDMFD R13!, {R6 - R8, R15}
