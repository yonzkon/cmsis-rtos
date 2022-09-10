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
    svc 0                         // to at_syscall
    LDMFD R13!, {R1 - R7, R15}

    .global at_syscall
at_syscall:
    STMFD R13!, {R6 - R8, R14}
    // set CONTROL[0] to 0
    MRS R8, CONTROL
    AND R8, #0b10
    MSR CONTROL, R8

    // FIXME: why interrupt may change r0, r1, r2 ?
    LDR R0, [SP, #16]
    LDR R1, [SP, #20]
    LDR R2, [SP, #24]
    LDR R3, [SP, #28]

//    // die if R0 changed somewhere
//    LDR R8, [SP, #16]
//    CMP R0, R8
//    IT NE
//die:
//    BNE die

    // call the syscall
    LDR R8, =syscall_table // R8 = syscall_tabl
    LSL R7, #2             // R7 *= 4
    LDR R6, [R8, R7]       // R6 = syscall_tabl + R7
    CMP R6, #0             // check if the handler is NULL
    IT NE
    BLXNE R6
    STR R0, [SP, #16]      // store r0 to stack which recover to r0

    // set CONTROL[0] to 1
    MRS R8, CONTROL
    ORR R8, #1
    MSR CONTROL, R8
    LDMFD R13!, {R6 - R8, R15}
