;ChipId: B6x

;Stack Configuration------------------------------------------------------------
Stack_Size      EQU     0x600
                AREA    STACK, NOINIT, READWRITE, ALIGN=3
Stack_Mem       SPACE   Stack_Size
__initial_sp
;-------------------------------------------------------------------------------

;Heap Configuration-------------------------------------------------------------
;Heap_Size       EQU     0x200
;                AREA    HEAP, NOINIT, READWRITE, ALIGN=3
;__heap_base
;Heap_Mem        SPACE   Heap_Size
;__heap_limit
;-------------------------------------------------------------------------------
                PRESERVE8
                THUMB

; Vector Table Mapped to Address 0 at Reset-------------------------------------
                AREA    RESET, DATA, READONLY
                EXPORT  __Vectors

__Vectors       DCD     __initial_sp           ; 0,  load top of stack
                DCD     Reset_Handler          ; 1,  Reset Handler
                DCD     NMI_Handler            ; 2,  NMI Handler
                DCD     HardFault_Handler      ; 3,  Hard Fault Handler
                DCD     0x18004000             ; LDR_RUN_ADDR
                DCD     0x18020000             ; LDR_INFO_ADDR

__Vectors_End

__Vectors_Size  EQU  __Vectors_End - __Vectors

;-------------------------------------------------------------------------------
                 AREA    |.INT|, CODE, READONLY 
                
;Reset Handler------------------------------------------------------------------
Reset_Handler   PROC
                EXPORT  Reset_Handler            [WEAK]
                IMPORT  __main

                LDR     R0, =__main
                BX      R0
                ENDP

; Dummy Exception Handlers (infinite loops here, can be modified)

NMI_Handler     PROC
                EXPORT  NMI_Handler              [WEAK]
                B       .
                ENDP

HardFault_Handler\
                PROC
                EXPORT  HardFault_Handler        [WEAK]
                B       .
                ENDP

                ALIGN
;*******************************************************************************
; User Stack and Heap initialization
;*******************************************************************************
                IF      :DEF:__MICROLIB

                EXPORT  __initial_sp
                ;EXPORT  __heap_base
                ;EXPORT  __heap_limit

                ELSE

                IMPORT  __use_two_region_memory
                EXPORT  __user_initial_stackheap
__user_initial_stackheap
                LDR     R0, =  Heap_Mem
                LDR     R1, = (Stack_Mem + Stack_Size)
                LDR     R2, = (Heap_Mem +  Heap_Size)
                LDR     R3, = Stack_Mem
                BX      LR

                ALIGN

                ENDIF

                END
