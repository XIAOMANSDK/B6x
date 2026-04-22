/**
 ****************************************************************************************
 *
 * @file ldo_test.c
 *
 * @brief LDO voltage regulator test - BOD (Brown-Out Detection) and LVD (Low Voltage Detection)
 *
 * @details
 * Test flow:
 * 1. Configure BOD and/or LVD thresholds via LDO registers
 * 2. Enable corresponding NVIC interrupts
 * 3. Monitor BOD/LVD events via ISR flags in main loop
 * 4. GPIO pulses on each event for oscilloscope observation
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "drvs.h"
#include "regs.h"
#include "dbg.h"


/*
 * DEFINES
 ****************************************************************************************
 */

/// GPIO pins for status indication
#define GPIO_BOD            GPIO04  ///< BOD event pulse indicator
#define GPIO_LVD            GPIO05  ///< LVD event pulse indicator
#define GPIO_RUN            GPIO15  ///< Main loop heartbeat indicator

/// BOD configuration mask and value
#define LDO_BOD_MSK         (LDO_BOD_ENB_MSK | LDO_BOD_TRIM_MSK)
#define LDO_BOD_CFG         (CFG_BOD_EN(1/*en*/, 1/*rst*/, 0/*irq*/) | CFG_BOD_TRIM(0))

/// LVD configuration mask and value
#define LDO_LVD_MSK         (LDO_LVD_ENB_MSK | LDO_LVD_SEL_MSK)
#define LDO_LVD_CFG         (CFG_LVD_EN(1/*en*/, 1/*rst*/, 0/*irq*/) | CFG_LVD_SEL(7))

/// Delay for LDO output to stabilize after configuration change
#define LDO_STABLE_MS       (15)

/// Heartbeat pulse duration for main loop indication
#define HEARTBEAT_MS        (2)

/*
 * VARIABLES
 ****************************************************************************************
 */

#if (BOD_TEST)
static volatile bool bodFlg    = false;  ///< BOD interrupt event flag
static volatile uint16_t bodCnt = 0;     ///< BOD interrupt event counter
#endif

#if (LVD_TEST)
static volatile bool lvdFlg    = false;  ///< LVD interrupt event flag
static volatile uint16_t lvdCnt = 0;     ///< LVD interrupt event counter
#endif

/*
 * FUNCTIONS
 ****************************************************************************************
 */

#if (BOD_TEST)

/**
 ****************************************************************************************
 * @brief BOD12 interrupt handler
 *
 * @details
 * Sets event flag and increments counter. GPIO pulse for scope observation.
 ****************************************************************************************
 */
void BOD12_IRQHandler(void)
{
    GPIO_DAT_SET(GPIO_BOD);

    bodFlg = true;
    bodCnt++;

    GPIO_DAT_CLR(GPIO_BOD);
}

/**
 ****************************************************************************************
 * @brief Process BOD event in main loop
 *
 * @details
 * Check and clear BOD event flag, log the event count.
 ****************************************************************************************
 */
static void bodEvent(void)
{
    if (bodFlg)
    {
        bodFlg = false;
        debug("BOD IRQ:%d\r\n", bodCnt);
    }
}
#endif

#if (LVD_TEST)

/**
 ****************************************************************************************
 * @brief LVD33 interrupt handler
 *
 * @details
 * Sets event flag and increments counter. GPIO pulse for scope observation.
 ****************************************************************************************
 */
void LVD33_IRQHandler(void)
{
    GPIO_DAT_SET(GPIO_LVD);

    lvdFlg = true;
    lvdCnt++;

    GPIO_DAT_CLR(GPIO_LVD);
}

/**
 ****************************************************************************************
 * @brief Process LVD event in main loop
 *
 * @details
 * Check and clear LVD event flag, log the event count.
 ****************************************************************************************
 */
static void lvdEvent(void)
{
    if (lvdFlg)
    {
        lvdFlg = false;
        debug("LVD IRQ:%d\r\n", lvdCnt);
    }
}
#endif

/**
 ****************************************************************************************
 * @brief Initialize LDO with BOD/LVD configuration
 *
 * @details
 * Read current LDO config, apply BOD and/or LVD settings, enable NVIC IRQs,
 * and wait for output stabilization.
 ****************************************************************************************
 */
static void ldoInit(void)
{
    uint32_t ldoCfg = core_ldoget();
    debug("LDO Get:0x%" PRIx32 "\r\n", ldoCfg);

    #if (BOD_TEST)
    /* External 1.2V reference for BOD */
    APBMISC->AON_PMU_CTRL.AON_PWR_SEL_RUN = 0;
    AON->PMU_CTRL.CORELDO_EN_RUN = 0;

    ldoCfg = (ldoCfg & ~LDO_BOD_MSK) | LDO_BOD_CFG;
    NVIC_EnableIRQ(BOD12_IRQn);
    #endif

    #if (LVD_TEST)
    ldoCfg = (ldoCfg & ~LDO_LVD_MSK) | LDO_LVD_CFG;
    NVIC_EnableIRQ(LVD33_IRQn);
    #endif

    core_ldoset(ldoCfg);
    bootDelayMs(LDO_STABLE_MS);

    debug("LDO Set:0x%" PRIx32 "\r\n", ldoCfg);
}

/**
 ****************************************************************************************
 * @brief LDO test main function
 *
 * @details
 * Initialize GPIO indicators and LDO, then continuously poll BOD/LVD events
 * with heartbeat GPIO pulse in main loop.
 ****************************************************************************************
 */
void ldoTest(void)
{
    GPIO_DIR_SET_LO(GPIO_BOD | GPIO_LVD | GPIO_RUN);

    GPIO_DAT_SET(GPIO_RUN);
    ldoInit();
    GPIO_DAT_CLR(GPIO_RUN);

    __enable_irq();

    while (1)
    {
        #if (BOD_TEST)
        bodEvent();
        #endif

        #if (LVD_TEST)
        lvdEvent();
        #endif

        GPIO_DAT_SET(GPIO_RUN);
        bootDelayMs(HEARTBEAT_MS);
        GPIO_DAT_CLR(GPIO_RUN);
    }
}
