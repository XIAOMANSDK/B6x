#ifndef _B6X_H_
#define _B6X_H_
/*
 * ==========================================================================
 * ---------- Interrupt Number Definition -----------------------------------
 * ==========================================================================
 */

typedef enum IRQn
{
/******  Cortex-M0 Processor Exceptions Numbers *************************************************/
    NMI_IRQn              = -14,      /*!< 2 Non Maskable Interrupt                               */
    HardFault_IRQn        = -13,      /*!< 3 Cortex-M0 Hard Fault Interrupt                       */
    SVCall_IRQn           = -5,       /*!< 11 Cortex-M0 SV Call Interrupt                         */
    PendSV_IRQn           = -2,       /*!< 14 Cortex-M0 Pend SV Interrupt                         */
    SysTick_IRQn          = -1,       /*!< 15 Cortex-M0 System Tick Interrupt                     */

    /******  Cortex-M0 specific Interrupt Numbers **********************************************/
    EXTI_IRQn             = 0,        /*  0       | EXTI          Interrupt                       */
    IWDT_IRQn             = 1,        /*  1       | IWDT          Interrupt                       */
    BLE_IRQn              = 2,        /*  2       | BLE           Interrupt                       */
    DMAC_IRQn             = 3,        /*  3       | DMAC          Interrupt                       */
    BB_LP_IRQn            = 4,        /*  4       | BB WAKEUP     Interrupt                       */
    BTMR_IRQn             = 5,        /*  5       | BTMR          Interrupt                       */
    CTMR_IRQn             = 6,        /*  6       | CTMR          Interrupt                       */
    ATMR_IRQn             = 7,        /*  7       | ATMR          Interrupt                       */
    RTC_IRQn              = 8,        /*  8       | RTC           Interrupt                       */
    I2C_IRQn              = 9,        /*  9       | I2C           Interrupt                       */
    SPIM_IRQn             = 10,       /*  10      | SPIM          Interrupt                       */
    SPIS_IRQn             = 11,       /*  11      | SPIS          Interrupt                       */
    UART1_IRQn            = 12,       /*  12      | UART1         Interrupt                       */
    UART2_IRQn            = 13,       /*  13      | UART2         Interrupt                       */
    AON_PMU_IRQn          = 14,       /*  14      | AON_PMU       Interrupt                       */
    LVD33_IRQn            = 15,       /*  15      | LVD33         Interrupt                       */
    BOD12_IRQn            = 16,       /*  16      | BOD12         Interrupt                       */
    USB_IRQn              = 17,       /*  17      | USB           Interrupt                       */
    USB_SOF_IRQn          = 18,       /*  18      | USB_SOF       Interrupt                       */
    FSHC_IRQn             = 19,       /*  19      | FSHC          Interrupt                       */
    MDM_IRQn              = 20,       /*  20      |               Interrupt                       */
    RF_IRQn               = 21,       /*  21      |               Interrupt                       */
} IRQn_Type;

#define __IRQFN            __attribute__((section("ram_func")))

/*
 * ==========================================================================
 * ----------- Processor and Core Peripheral Section ------------------------
 * ==========================================================================
 */

/* Configuration of the Cortex-M0 Processor and Core Peripherals */
#define __MPU_PRESENT           0       /*!< cm0ikmcu does not provide a MPU present or not       */
#define __NVIC_PRIO_BITS        2       /*!< cm0ikmcu Supports 2 Bits for the Priority Levels     */
#define __Vendor_SysTickConfig  0       /*!< Set to 1 if different SysTick Config is used         */

#if (1)
#define __VTOR_PRESENT 1
#include "core_cm0plus.h"               /* Cortex-M0 plus processor and core peripherals               */
#else
#include "core_cm0.h"                   /* Cortex-M0 processor and core peripherals               */
#endif

#if   defined ( __CC_ARM )
#pragma anon_unions
#pragma diag_suppress 1296
#endif

/* SYSTICK - Cortex-M0 SysTick Register */
typedef struct
{
    __IO uint32_t   CSR;            //0x0, Control and Status
    __IO uint32_t   RVR;            //0x4, Reload Value
    __IO uint32_t   CVR;            //0x8, Current Value
    __IO uint32_t   CALIB;          //0xC, Calibration
} TICK_TypeDef;


/***************************************************************************/
/*                         Peripheral Memory map                           */
/***************************************************************************/
#define BOOTROM_BASE      ((uint32_t)0x00000000) // Boot Memory       (4KB)
#define FLASH_BASE        ((uint32_t)0x18000000) // FLASH Controller  (16MB)
#define CACHE_REG_BASE    ((uint32_t)0x19000000) // CACHE Reg         (4KB)
#define SRAM_BASE         ((uint32_t)0x20003000) // SRAM Memory       (20KB)
#define RETN_BASE         ((uint32_t)0x20008000) // BLE EM Memory     (8KB)
#define AHB_BASE          ((uint32_t)0x40000000) // AHB Peripheral    (64KB)
#define APB1_BASE         ((uint32_t)0x40020000) // APB Peripheral    (64KB)
#define APB2_BASE         ((uint32_t)0x40030000) // APB Peripheral    (64KB)

/*
 * ==========================================================================
 * ----------------------------  Common MACRO  ------------------------------
 * ==========================================================================
 */

/** @brief Enable interrupts globally in the system.
 * This macro must be used when the initialization phase is over and the interrupts
 * can start being handled by the system.
 */
#define GLOBAL_INT_START()    __enable_irq() //__set_PRIMASK(0)

/** @brief Disable interrupts globally in the system.
 * This macro must be used when the system wants to disable all the interrupt
 * it could handle.
 */
#define GLOBAL_INT_STOP()     __disable_irq() //__set_PRIMASK(1)

/** @brief Disable interrupts globally in the system.
 * This macro must be used in conjunction with the @ref GLOBAL_INT_RESTORE macro since this
 * last one will close the brace that the current macro opens.  This means that both
 * macros must be located at the same scope level.
 */
#define GLOBAL_INT_DISABLE()                  \
do {                                          \
    uint32_t __l_irq_rest = __get_PRIMASK();  \
    __disable_irq();

/** @brief Restore interrupts from the previous global disable.
 * @sa GLOBAL_INT_DISABLE
 */
//if (__l_irq_rest == 0) __enable_irq();    
#define GLOBAL_INT_RESTORE()                  \
    __set_PRIMASK(__l_irq_rest);              \
} while(0)

#define WR_8(addr,value)  (*(volatile uint8_t  *)(addr)) = (value)
#define WR_32(addr,value) (*(volatile uint32_t *)(addr)) = (value)
#define RD_32(addr)       (*(volatile uint32_t *)(addr))

#include "rom.h"

#endif //_B6X_H_
