/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief PWC (Pulse Width Capture) test - timer input capture with frequency and duty measurement
 *
 * @details
 * Test flow:
 * 1. Configure timer channel in input capture mode (CTMR or ATMR selectable)
 * 2. Capture both rising and falling edges (dual-edge mode)
 * 3. Calculate pulse width, period, frequency and duty cycle
 * 4. Output measurement results via debug interface
 *
 * Timer configuration:
 * - Clock: 16 MHz, prescaler 15999 -> 1 kHz count frequency (1 ms resolution)
 * - Dual-edge capture uses TRC input with TI1F_ED trigger + reset mode
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

/// GPIO pins
#define PA_RET_SEE1           (9)   ///< ISR timing indicator pin
#define PA_CAP_CH1            (16)  ///< Capture channel 1 input pin (CTMR)

/// Edge trigger mode: 1=rising only, 2=falling only, 3=dual edge
#define POS_NEG_EDGE          (3)

/// Timer prescaler: 16 MHz / (15999+1) = 1 kHz count frequency
#define PWC_PRESCALER         (15999)

/// Count frequency in Hz (derived from timer clock / (prescaler + 1))
#define PWC_COUNT_FREQ_HZ    (1000.0f)

#if (POS_NEG_EDGE == 1)
#define PA_TMR_EDGE           (PWC_CCER_POSEDGE)
#define PWC_CC1S              (1)
#define PWC_SMCR_TS_SMS       (0x54)  ///< SMCR.TS=5(TI1FP1), SMS=4(reset mode)
#elif (POS_NEG_EDGE == 2)
#define PA_TMR_EDGE           (PWC_CCER_NEGEDGE)
#define PWC_CC1S              (1)
#define PWC_SMCR_TS_SMS       (0x54)
#elif (POS_NEG_EDGE == 3)
#define PA_TMR_EDGE           (PWC_CCER_POSEDGE | PWC_CCER_NEGEDGE)
#define PWC_CC1S              (3)     ///< TRC as IC1 input for dual-edge
#define PWC_SMCR_TS_SMS       (0x44)  ///< SMCR.TS=4(TI1F_ED), SMS=4(reset mode)
#endif

/// Timer selection: CTMR_USED=1 -> CTMR, CTMR_USED=0 -> ATMR
#if (CTMR_USED)
#define PWC_TMR               (PWM_CTMR)
#define PWC_TMR_CH(n)         (PWM_CTMR_CH##n)
#define PWC_IRQc              (CTMR_IRQn)
#define PA_PWC_CH1            (16)
#else
#define PWC_TMR               (PWM_ATMR)
#define PWC_TMR_CH(n)         (PWM_ATMR_CH##n##P)
#define PWC_IRQc              (ATMR_IRQn)
#define PA_PWC_CH1            (7)
#endif

/// Timer interrupt flag bits
#define TIMER_INT_CH1_BIT     (0x02U)
#define TIMER_INT_CH2_BIT     (0x04U)
#define TIMER_INT_CH3_BIT     (0x08U)
#define TIMER_INT_CH4_BIT     (0x10U)

/*
 * TYPES
 ****************************************************************************************
 */

/// Capture measurement result
typedef struct {
    uint32_t high_width;    ///< High-level width in timer counts
    uint32_t low_width;     ///< Low-level width in timer counts
    uint32_t period;        ///< Signal period in timer counts
    float frequency;        ///< Signal frequency in Hz
    float duty_cycle;       ///< Duty cycle in percentage
    uint32_t last_capture;  ///< Previous capture value
} capture_result_t;

/*
 * VARIABLES
 ****************************************************************************************
 */

static volatile capture_result_t g_cap_result = {0};

/*
 * FUNCTIONS
 ****************************************************************************************
 */

#if (CTMR_USED)

/**
 ****************************************************************************************
 * @brief CTMR interrupt handler for input capture
 *
 * @details
 * Reads CCR1 on each capture event, determines edge polarity from pin level,
 * and updates high/low width, period, frequency and duty cycle.
 ****************************************************************************************
 */
void CTMR_IRQHandler(void)
{
    uint32_t iflg = CTMR->IFM.Word;

    if (iflg & TIMER_INT_CH1_BIT)
    {
        GPIO_DAT_SET(1 << PA_RET_SEE1);

        /* CCR+1 compensates for counter sync latency */
        uint32_t cur = CTMR->CCR1 + 1;
        uint32_t diff = cur - g_cap_result.last_capture;

        /* Pin high -> rising edge just occurred -> previous interval was low width */
        if (gpio_get(PA_PWC_CH1) ^ 0x01)
        {
            g_cap_result.low_width = diff;
        }
        else
        {
            g_cap_result.high_width = diff;
        }

        g_cap_result.period = g_cap_result.high_width + g_cap_result.low_width;
        if (g_cap_result.period > 0)
        {
            g_cap_result.frequency = PWC_COUNT_FREQ_HZ / g_cap_result.period;
            g_cap_result.duty_cycle = (float)g_cap_result.high_width / g_cap_result.period * 100.0f;
        }

        g_cap_result.last_capture = cur;

        debug("TIME:[%" PRIu32 " ms] LEVEL:[%d] H:%" PRIu32 " L:%" PRIu32
              " P:%" PRIu32 " F:%.2fHz D:%.1f%%\r\n",
              cur, gpio_get(PA_PWC_CH1) ^ 0x01,
              g_cap_result.high_width, g_cap_result.low_width,
              g_cap_result.period, g_cap_result.frequency,
              g_cap_result.duty_cycle);

        CTMR->ICR.CC1I = 1;

        GPIO_DAT_CLR(1 << PA_RET_SEE1);
    }
}
#else

/**
 ****************************************************************************************
 * @brief ATMR interrupt handler for input capture
 ****************************************************************************************
 */
void ATMR_IRQHandler(void)
{
    uint32_t iflg = ATMR->IFM.Word;

    if (iflg & TIMER_INT_CH1_BIT)
    {
        GPIO_DAT_SET(1 << PA_RET_SEE1);

        ATMR->ICR.CC1I = 1;

        GPIO_DAT_CLR(1 << PA_RET_SEE1);
    }
}
#endif

/**
 ****************************************************************************************
 * @brief Initialize and run timer capture test
 *
 * @details
 * Configure GPIO, timer prescaler, capture channel (CCMR/CCER), slave mode
 * (SMCR reset on trigger), and enable NVIC interrupt.
 ****************************************************************************************
 */
static void captureTest(void)
{
    GPIO_DAT_CLR((1 << PA_RET_SEE1) | (1 << PA_PWC_CH1));
    GPIO_DIR_SET((1 << PA_RET_SEE1));
    GPIO_DIR_CLR((1 << PA_PWC_CH1));

    #if (CTMR_USED)
    iom_ctrl(PA_PWC_CH1, IOM_PULLDOWN | IOM_INPUT | IOM_SEL_CSC);
    csc_input(PA_PWC_CH1, CSC_CTMR_CH1);
    #else
    iom_ctrl(PA_PWC_CH1, IOM_PULLDOWN | IOM_INPUT | IOM_SEL_TIMER);
    #endif

    pwm_init(PWC_TMR, PWC_PRESCALER, UINT16_MAX);

    pwm_chnl_cfg_t chnl_cfg;
    chnl_cfg.duty = 0;
    chnl_cfg.ccmr = PWC_CCMR_MODE(PWC_CC1S, 3, PWC_PSC0);

    #if (POS_NEG_EDGE != 3)
    chnl_cfg.ccer = PA_TMR_EDGE;
    #endif

    pwm_chnl_set(PWC_TMR_CH(1), &chnl_cfg);

    pwm_conf(PWC_TMR, PWC_SMCR_TS_SMS, TIMER_INT_CH1_BIT | TIMER_INT_CH2_BIT);

    pwm_start(PWC_TMR);

    NVIC_EnableIRQ(PWC_IRQc);
    __enable_irq();
}

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
 * @brief Device initialization
 ****************************************************************************************
 */
static void devInit(void)
{
    iwdt_disable();

    dbgInit();
    debug("Capture Test...\r\n");
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

    captureTest();

    while (1)
    {
    }
}
