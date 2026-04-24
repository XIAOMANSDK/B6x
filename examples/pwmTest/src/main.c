/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Main Entry of the application - PWM output test
 *
 * @details
 * Demonstrates PWM output with CTMR/ATMR and optional DMA duty-cycle updates.
 * PWM freq = sys_clk / (PSC+1) / (ARR+1) = 16MHz / 16 / 100 = 10kHz
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "drvs.h"
#include "dbg.h"


/*
 * DEFINES
 ****************************************************************************************
 */

#define PWM_TMR_PSC            (16 - 1)   ///< Prescaler: 16MHz -> 1MHz
#define PWM_TMR_ARR            (100 - 1)  ///< Auto-reload: 1MHz / 100 = 10kHz

/// Timer selection: CTMR (general) or ATMR (advanced)
#if (CTMR_USED)
#define PA_CTMR_CH1            (15)
#define PA_CTMR_CH2            (16)
#define PA_CTMR_CH3            (17)
#define PA_CTMR_CH4            (18)
#else
#define PA_ATMR_CH1P           (7)
#define PA_ATMR_CH2P           (8)
#define PA_ATMR_CH3P           (9)
#define PA_ATMR_CH4P           (10)
#define PA_ATMR_CH1N           (11)
#define PA_ATMR_CH2N           (12)
#define PA_ATMR_CH3N           (13)
#endif

/// DMA configuration
#if (DMA_USED)
#if (CTMR_USED)
#define PWM_CTMR_DMA_CHNL     (DMA_CH0)
#else
#define PWM_ATMR_DMA_CHNL     (DMA_CH1)
#endif
#define PWM_DUTY_CNT          (10)

static uint32_t pwm_duty_buff0[PWM_DUTY_CNT];  ///< DMA primary buffer
UNUSED static uint32_t pwm_duty_buff1[PWM_DUTY_CNT];  ///< DMA alternate buffer

#define PA_DONE_SEE           (2)
#endif

/// Output polarity with optional DMA request enable
#if (DMA_USED)
#define CFG_PWM_CCER_SIPH      (PWM_CCER_SIPH | PWM_CCxDE_BIT)
#define CFG_PWM_CCER_SIPL      (PWM_CCER_SIPL | PWM_CCxDE_BIT)
#else
#define CFG_PWM_CCER_SIPH      (PWM_CCER_SIPH)
#define CFG_PWM_CCER_SIPL      (PWM_CCER_SIPL)
#endif


/*
 * FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief PWM test: configure channels with different duty cycles and polarities
 *
 * @details
 * CTMR mode: 4 independent channels (15%, 25%, 35%, 45%)
 * ATMR mode: 3 complementary pairs + 1 single-ended
 * DMA mode:  auto-updates duty cycle from buffer
 ****************************************************************************************
 */
static void pwm_test(void)
{
    #if (DMA_USED)
    uint16_t idx = 0;

    GPIO_DIR_SET_LO(1 << PA_DONE_SEE);
    dma_init();

    // Fill duty buffers: buff0 ascending 10%~100%, buff1 descending 100%~10%
    for (idx = 0; idx < PWM_DUTY_CNT; idx++)
    {
        pwm_duty_buff0[idx] = (idx + 1) * 10 - 1;
        pwm_duty_buff1[idx] = (PWM_DUTY_CNT - idx) * 10 - 1;
    }
    #endif

    // Configure PWM output pins
    #if (CTMR_USED)
    iom_ctrl(PA_CTMR_CH1, IOM_SEL_TIMER);
    iom_ctrl(PA_CTMR_CH2, IOM_SEL_TIMER);
    iom_ctrl(PA_CTMR_CH3, IOM_SEL_TIMER);
    iom_ctrl(PA_CTMR_CH4, IOM_SEL_TIMER);
    #else
    iom_ctrl(PA_ATMR_CH1P, IOM_SEL_TIMER);
    iom_ctrl(PA_ATMR_CH2P, IOM_SEL_TIMER);
    iom_ctrl(PA_ATMR_CH3P, IOM_SEL_TIMER);
    iom_ctrl(PA_ATMR_CH4P, IOM_SEL_TIMER);
    iom_ctrl(PA_ATMR_CH1N, IOM_SEL_TIMER);
    iom_ctrl(PA_ATMR_CH2N, IOM_SEL_TIMER);
    iom_ctrl(PA_ATMR_CH3N, IOM_SEL_TIMER);
    #endif

    pwm_chnl_cfg_t chnl_conf;
    chnl_conf.ccmr = PWM_CCMR_MODE1;

    #if (CTMR_USED)
    pwm_init(PWM_CTMR, PWM_TMR_PSC, PWM_TMR_ARR);

    chnl_conf.duty = 15;
    chnl_conf.ccer = CFG_PWM_CCER_SIPH;
    pwm_chnl_set(PWM_CTMR_CH1, &chnl_conf);

    chnl_conf.duty = 25;
    chnl_conf.ccer = CFG_PWM_CCER_SIPL;
    pwm_chnl_set(PWM_CTMR_CH2, &chnl_conf);

    chnl_conf.duty = 35;
    chnl_conf.ccer = CFG_PWM_CCER_SIPH;
    pwm_chnl_set(PWM_CTMR_CH3, &chnl_conf);

    chnl_conf.duty = 45;
    chnl_conf.ccer = CFG_PWM_CCER_SIPL;
    pwm_chnl_set(PWM_CTMR_CH4, &chnl_conf);

    pwm_start(PWM_CTMR);
    #else
    // ATMR: complementary PWM pairs
    pwm_init(PWM_ATMR, PWM_TMR_PSC, PWM_TMR_ARR);

    chnl_conf.duty = 10;
    chnl_conf.ccer = CFG_PWM_CCER_SIPH;
    pwm_chnl_set(PWM_ATMR_CH1P, &chnl_conf);
    pwm_chnl_set(PWM_ATMR_CH1N, &chnl_conf);

    chnl_conf.duty = 20;
    chnl_conf.ccer = CFG_PWM_CCER_SIPL;
    pwm_chnl_set(PWM_ATMR_CH2P, &chnl_conf);
    pwm_chnl_set(PWM_ATMR_CH2N, &chnl_conf);

    chnl_conf.duty = 30;
    chnl_conf.ccer = CFG_PWM_CCER_SIPH;
    pwm_chnl_set(PWM_ATMR_CH3P, &chnl_conf);
    pwm_chnl_set(PWM_ATMR_CH3N, &chnl_conf);

    chnl_conf.duty = 40;
    chnl_conf.ccer = CFG_PWM_CCER_SIPL;
    pwm_chnl_set(PWM_ATMR_CH4P, &chnl_conf);

    pwm_start(PWM_ATMR);
    #endif

    #if (DMA_USED)
    #if (CTMR_USED)
    DMA_CTMR_CHx_INIT(PWM_CTMR_DMA_CHNL, 4);
    DMA_CTMR_CHx_CONF(PWM_CTMR_DMA_CHNL, 4, pwm_duty_buff0, PWM_DUTY_CNT, CCM_BASIC);
    //DMA_CTMR_CHx_CONF(PWM_CTMR_DMA_CHNL | DMA_CH_ALT, 4, pwm_duty_buff1, PWM_DUTY_CNT, CCM_PING_PONG);
    #else
    DMA_ATMR_CHx_INIT(PWM_ATMR_DMA_CHNL, 1);
    DMA_ATMR_CHx_CONF(PWM_ATMR_DMA_CHNL, 1, pwm_duty_buff0, PWM_DUTY_CNT, CCM_PING_PONG);
    DMA_ATMR_CHx_CONF(PWM_ATMR_DMA_CHNL | DMA_CH_ALT, 1, pwm_duty_buff1, PWM_DUTY_CNT, CCM_PING_PONG);
    #endif

    while (1)
    {
        #if (CTMR_USED)
        if (dma_chnl_done(PWM_CTMR_DMA_CHNL))
        {
            GPIO_DAT_SET(1 << PA_DONE_SEE);
            dma_chnl_reload(PWM_CTMR_DMA_CHNL);
            GPIO_DAT_CLR(1 << PA_DONE_SEE);
        }
        #else
        if (dma_chnl_done(PWM_ATMR_DMA_CHNL))
        {
            GPIO_DAT_SET(1 << PA_DONE_SEE);
            dma_chnl_reload(PWM_ATMR_DMA_CHNL);
            GPIO_DAT_CLR(1 << PA_DONE_SEE);
        }
        #endif

        //idx = (1 + idx) % PWM_DUTY_CNT;
        //pwm_duty_upd(PWM_CTMR_CH1, pwm_duty_buff0[idx]);
    }
    #endif
}

/**
 ****************************************************************************************
 * @brief System initialization (template placeholder)
 ****************************************************************************************
 */
static void sys_init(void)
{
    // Todo config, if need
}

/**
 ****************************************************************************************
 * @brief Device initialization
 *
 * @details Disable watchdog, initialize debug interface
 ****************************************************************************************
 */
static void dev_init(void)
{
    iwdt_disable();

    dbgInit();
    debug("PWM Test...\r\n");
}

/**
 ****************************************************************************************
 * @brief Main entry
 * @return Program exit code (never returns)
 ****************************************************************************************
 */
int main(void)
{
    sys_init();
    dev_init();

    pwm_test();

    while (1);
}
