    .syntax unified
    .arch   armv6-m

    .extern main
    .extern trim_load
    .extern core_vector

    .section .stack
    .align  3
#ifdef __STACK_SIZE
    .equ    Stack_Size, __STACK_SIZE
#else
    .equ    Stack_Size, 0x1000
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
    .long   0                     /* Reserved */
    .long   0                     /* Reserved */
    .long   0                     /* Reserved */
    .long   0                     /* Reserved */
    .long   0                     /* Reserved */
    .long   0                     /* Reserved */
    .long   0                     /* Reserved */
    .long   SVC_Handler           /* SVCall Handler */
    .long   0                     /* Reserved */
    .long   0                     /* Reserved */
    .long   PendSV_Handler        /* PendSV Handler */
    .long   SysTick_Handler       /* SysTick Handler */

    /* External interrupts */
    .long   EXTI_IRQHandler       /* 0  */
    .long   IWDT_IRQHandler       /* 1  */
    .long   BLE_IRQHandler        /* 2  */
    .long   DMAC_IRQHandler       /* 3  */
    .long   BB_LP_IRQHandler      /* 4  */
    .long   BTMR_IRQHandler       /* 5  */
    .long   CTMR_IRQHandler       /* 6  */
    .long   ATMR_IRQHandler       /* 7  */
    .long   RTC_IRQHandler        /* 8  */
    .long   I2C_IRQHandler        /* 9  */
    .long   SPIM_IRQHandler       /* 10 */
    .long   SPIS_IRQHandler       /* 11 */
    .long   UART1_IRQHandler      /* 12 */
    .long   UART2_IRQHandler      /* 13 */
    .long   AON_PMU_IRQHandler    /* 14 */
    .long   LVD33_IRQHandler      /* 15 */
    .long   BOD12_IRQHandler      /* 16 */
    .long   USB_IRQHandler        /* 17 */
    .long   USB_SOF_IRQHandler    /* 18 */
    .long   FSHC_IRQHandler       /* 19 */
    .long   MDM_IRQHandler        /* 20 */
    .long   RF_IRQHandler         /* 21 */
    .size    __isr_vector, . - __isr_vector

    .text
    .thumb
    .thumb_func
    .align    1
    .globl    Reset_Handler
    .type    Reset_Handler, %function
    .section ".text.user_irq_handler"
Reset_Handler:

    /*
     * cache cfg
     * SYSCFG->CACHSRAM_CFG = 0;
     */
    movs    r0, #0x00
    ldr     r1, =0x4000102c
    str     r0, [r1]

    /* cache->crcr0.word = 0x18004025; */
    ldr     r0, =0x18004025
    ldr     r1, =0x1900000c
    str     r0, [r1]

    /* cache->ccr.cache_en = 1; */
    movs    r0, #0x01
    subs    r1, #0x0c
    str     r0, [r1]

    /*
     * APBMISC->XOSC16M_CTRL.Word = 0x00014894;
     * .XOSC16M_LP=0 .XOSC16M_CAP_TR=0x14
     */
    ldr     r0, =0x00014894
    ldr     r1, =0x40031054
    str     r0, [r1]

    ldr     r1, =__etext
    ldr     r2, =__data_start__
    ldr     r3, =__data_end__
    subs    r3, r2
    ble    .L_loop1_done

.L_loop1:
    subs    r3, #4
    ldr     r0, [r1,r3]
    str     r0, [r2,r3]
    bgt    .L_loop1

.L_loop1_done:
/*  Single BSS section scheme.
 *
 *  The BSS section is specified by following symbols
 *    __bss_start__: start of the BSS section.
 *    __bss_end__: end of the BSS section.
 *
 *  Both addresses must be aligned to 4 bytes boundary.
 */
    ldr     r1, =__bss_start__
    ldr     r2, =__bss_end__

    movs    r0, 0

    subs    r2, r1
    ble    .L_loop3_done

.L_loop3:
    subs    r2, #4
    str     r0, [r1, r2]
    bgt    .L_loop3

.L_loop3_done:
    ldr   r0, =_stext
    bl    core_vector
    bl    trim_load
    bl    main

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

    .size   HardFault_Handler, . - HardFault_Handler
    .weak   SVC_Handler
    .type   SVC_Handler, %function
SVC_Handler:
    b       .

    .size   SVC_Handler, . - SVC_Handler
    .weak   PendSV_Handler
    .type   PendSV_Handler, %function
PendSV_Handler:
    b       .

    .size   PendSV_Handler, . - PendSV_Handler
    .weak   SysTick_Handler
    .type   SysTick_Handler, %function
SysTick_Handler:
    b       .

    .size   SysTick_Handler, . - SysTick_Handler
    .globl  Default_Handler
    .type   Default_Handler, %function
Default_Handler:
    b       .
    .size    Default_Handler, . - Default_Handler

/*    Macro to define default handlers. Default handler
 *    will be weak symbol and just dead loops. They can be
 *    overwritten by other handlers */
    .macro  def_irq_handler    handler_name
    .weak   \handler_name
    .set    \handler_name, Default_Handler
    .endm

    def_irq_handler EXTI_IRQHandler       /* 0 */
    def_irq_handler IWDT_IRQHandler       /* 1 */
    def_irq_handler BLE_IRQHandler        /* 2 */
    def_irq_handler DMAC_IRQHandler       /* 3 */
    def_irq_handler BB_LP_IRQHandler      /* 4 */
    def_irq_handler BTMR_IRQHandler       /* 5 */
    def_irq_handler CTMR_IRQHandler       /* 6 */
    def_irq_handler ATMR_IRQHandler       /* 7 */
    def_irq_handler RTC_IRQHandler        /* 8 */
    def_irq_handler I2C_IRQHandler        /* 9 */
    def_irq_handler SPIM_IRQHandler       /* 10*/
    def_irq_handler SPIS_IRQHandler       /* 11 */
    def_irq_handler UART1_IRQHandler      /* 12 */
    def_irq_handler UART2_IRQHandler      /* 13 */
    def_irq_handler AON_PMU_IRQHandler    /* 14 */
    def_irq_handler LVD33_IRQHandler      /* 15 */
    def_irq_handler BOD12_IRQHandler      /* 16 */
    def_irq_handler USB_IRQHandler        /* 17 */
    def_irq_handler USB_SOF_IRQHandler    /* 18 */
    def_irq_handler FSHC_IRQHandler       /* 19 */
    def_irq_handler MDM_IRQHandler        /* 20 */
    def_irq_handler RF_IRQHandler         /* 21 */

    .end
