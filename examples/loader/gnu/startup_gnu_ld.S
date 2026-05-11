    .syntax unified
    .arch   armv6-m

    .extern __main

    .section .stack
    .align  3
#ifdef __STACK_SIZE
    .equ    Stack_Size, __STACK_SIZE
#else
    .equ    Stack_Size, 0x600
#endif

    .globl  __StackTop
    .globl  __StackLimit
__StackLimit:
    .space  Stack_Size
    .size   __StackLimit, . - __StackLimit
__StackTop:
    .size   __StackTop, . - __StackTop

    .section .heap
    .align  3

    .equ    Heap_Size, 0

    .globl  __HeapBase
    .globl  __HeapLimit
__HeapBase:
    .if    Heap_Size
    .space  Heap_Size
    .endif
    .size   __HeapBase, . - __HeapBase
__HeapLimit:
    .size   __HeapLimit, . - __HeapLimit

    .section .isr_vector
    .align 2
    .globl  __isr_vector
__isr_vector:
    .long   __StackTop            /* Top of Stack */
    .long   Reset_Handler         /* Reset Handler */
    .long   NMI_Handler           /* NMI Handler */
    .long   HardFault_Handler     /* Hard Fault Handler */
    .long   0x18004000            /* LDR_RUN_ADDR */
    .long   0x18001F00            /* LDR_INFO_ADDR */
    .size    __isr_vector, . - __isr_vector

    .text
    .thumb
    .thumb_func
    .align    1
    .globl    Reset_Handler
    .type    Reset_Handler, %function
    .section ".text.user_irq_handler"
Reset_Handler:

    bl      __main

    .pool
    .size    Reset_Handler, . - Reset_Handler

/* Dummy Exception Handlers (infinite loops which can be modified) */

    .weak   NMI_Handler
    .type   NMI_Handler, %function
NMI_Handler:
    b       .

    .size   NMI_Handler, . - NMI_Handler
    .weak   HardFault_Handler
    .type   HardFault_Handler, %function
HardFault_Handler:
    b       .

    .end
