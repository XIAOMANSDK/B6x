/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Fault exception handler test - triggers HardFault via invalid pointer access
 *
 * @details
 * Test flow:
 * 1. Initialize trace (fault handler) and debug interface
 * 2. Read from NULL address (0x00) to trigger HardFault
 * 3. Read from aligned (0x04) and unaligned (0x03) addresses
 * 4. Fault handler (trace_init) captures LR/SP for post-mortem debug
 *
 * Note: Execution typically stops at the first fault unless the handler
 * performs stack manipulation to resume.
 *
 ****************************************************************************************
 */

#include "drvs.h"
#include "dbg.h"


/*
 * DEFINES
 ****************************************************************************************
 */

#if (DBG_MODE != DBG_VIA_UART)
#error "Trace info output by printf()."
#endif


/*
 * FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief System initialization
 ****************************************************************************************
 */
static void sysInit(void)
{
    // TODO: Add system clock configuration if needed
}

/**
 ****************************************************************************************
 * @brief Device initialization with fault context capture
 *
 * @details
 * Capture LR and SP values for fault diagnosis, then print reset reason
 * and stack context for post-mortem analysis.
 ****************************************************************************************
 */
static void devInit(void)
{
    uint16_t rsn = rstrsn();

    iwdt_disable();

    dbgInit();

    #if (COMPILER_AC5)
    uint32_t fault_lr = __return_address();
    uint32_t fault_sp = __current_sp();
    #else
    uint32_t fault_lr = (uint32_t)__builtin_return_address(0);
    uint32_t fault_sp;
    __asm volatile ("mov %0, sp" : "=r" (fault_sp));
    #endif

    debug("Start(rsn:0x%X)...\r\n", rsn);
    uint32_t msp = __get_MSP();
    debug("%" PRIx32 ", %" PRIx32 ", %" PRIx32 "...\r\n", fault_lr, fault_sp, msp);
}

/**
 ****************************************************************************************
 * @brief User procedure (placeholder)
 ****************************************************************************************
 */
static void userProc(void)
{
    // TODO: Add user procedure if needed
}

/**
 ****************************************************************************************
 * @brief Trigger fault exceptions by reading invalid/unaligned addresses
 *
 * @details
 * Reads from NULL (0x00), aligned (0x04), and unaligned (0x03) addresses
 * to exercise the HardFault handler. GCC diagnostic pragmas suppress
 * array-bounds warnings for these intentional violations.
 ****************************************************************************************
 */
void faultTest(void)
{
    trace_init();

    volatile int *p;
    volatile int value;

    p = (int *)0x00;
    value = *p;
    debug("addr:0x%02X value:0x%08X\r\n", (int) p, value);

    #if (COMPILER_GCC)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Warray-bounds"
    #endif

    p = (int *)0x04;
    value = *p;
    debug("addr:0x%02X value:0x%08X\r\n", (int) p, value);

    p = (int *)0x03;
    value = *p;

    #if (COMPILER_GCC)
    #pragma GCC diagnostic pop
    #endif

    debug("addr:0x%02X value:0x%08X\r\n", (int) p, value);
}

/**
 ****************************************************************************************
 * @brief Application entry point
 ****************************************************************************************
 */
int main(void)
{
    sysInit();
    devInit();

    faultTest();

    while (1)
    {
        userProc();
    }
}
