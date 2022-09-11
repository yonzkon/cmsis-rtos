    .syntax unified
    .cpu cortex-m3
    .fpu softvfp
    .thumb

    .extern current

    .global reset_msp
reset_msp:
    MOV R0, #0
    LDR R0, [R0, #0]
    MSR MSP, R0
    MOV PC, LR

    .global switch_to_user_task
switch_to_user_task:
    // set psp
    LDR R1, [R0, #68]
    MSR PSP, R1

    // set current
    LDR R1, =current
    STR R0, [R1]

    // move to user mode
    MRS R1, CONTROL
    ORR R1, #3
    MSR CONTROL, R1 // if control is 3, this code will not take effect

    // b entry
    LDR R0, [R0, #60] // PC
    bx R0

    .global switch_to
switch_to:
    PUSH {LR}

    // save prev current {R4~R11}
    LDR R1, =current
    LDR R1, [R1]
    STR R4, [R1, #16]
    STR R5, [R1, #20]
    STR R6, [R1, #24]
    STR R7, [R1, #28]
    STR R8, [R1, #32]
    STR R9, [R1, #36]
    STR R10, [R1, #40]
    STR R11, [R1, #44]
    MRS R2, PSP
    STR R2, [R1, #68]

    // set to next current
    LDR R1, =current
    STR R0, [R1]

    // {R0~R3, R12, PC, LR, XPSR} will be recover by iret, SP can ignore
    // recover next current {R4~R11}
    LDR R4, [R0, #16]
    LDR R5, [R0, #20]
    LDR R6, [R0, #24]
    LDR R7, [R0, #28]
    LDR R8, [R0, #32]
    LDR R9, [R0, #36]
    LDR R10, [R0, #40]
    LDR R11, [R0, #44]

    // set psp
    LDR R1, [R0, #68]
    MSR PSP, R1

    POP {PC}
