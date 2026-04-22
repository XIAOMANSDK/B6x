
    AREA    CtrlFunc, CODE
    IMPORT  task_sys_ctrl
    IMPORT  task_current
    IMPORT  task_next
    EXPORT  InvokeTask
    EXPORT  PendSV_Handler

InvokeTask
    cpsid i
    ldr r1, =task_sys_ctrl
    ldr r0, [r1]
    adds r0, #32
    msr psp, r0
    movs r0, #2
    msr CONTROL, r0
    isb
    pop {r0-r5}
    mov lr, r5
    pop {r3}
    pop {r2}
    cpsie i
    bx r3

PendSV_Handler
    cpsid i

    mrs r0, psp

    ldr r3, =task_current
    ldr r2, [r3]

    subs r0, r0, #32
    str r0, [r2]

    stmia r0!, {r4-r7}
    mov r4, r8
    mov r5, r9
    mov r6, r10
    mov r7, r11
    stmia r0!, {r4-r7}

    ldr r2, =task_next
    ldr r1, [r2]

    ldr r0, [r1]
    str r1, [r3]

    adds r0, r0, #16
    ldmia r0!, {r4-r7}
    mov r8, r4
    mov r9, r5
    mov r10, r6
    mov r11, r7

    msr psp, r0

    subs r0, r0, #32
    ldmia r0!, {r4-r7}

    cpsie i

    bx lr

    END