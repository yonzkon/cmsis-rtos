    .syntax unified
    .cpu cortex-m3
    .fpu softvfp
    .thumb

    .extern syscall_table

    .global __syscall
__syscall:
    STMFD R13!, {R1 - R7, R14}
    LDR R4, [SP, #32]
    LDR R5, [SP, #36]
    LDR R6, [SP, #40]
    MOV R7, R6
    svc 0                         // to at_syscall
    LDMFD R13!, {R1 - R7, R15}

    .global at_syscall
at_syscall:
    STMFD R13!, {R6 - R8, R14}

    // FIXME: why interrupt may change r0, r1, r2 ?
    MRS R6, PSP
    LDR R0, [R6, #0]
    LDR R1, [R6, #4]
    LDR R2, [R6, #8]
    LDR R3, [R6, #12]

    // call the syscall
    LDR R6, =syscall_table // R8 = syscall_tabl
    LSL R7, #2             // R7 *= 4
    LDR R6, [R6, R7]       // R6 = syscall_tabl + R7
    CMP R6, #0             // check if the handler is NULL
    IT NE
    BLXNE R6
    MRS R6, PSP
    STR R0, [R6, #0]      // store r0 to stack which recover to r0

    LDMFD R13!, {R6 - R8, R15}
