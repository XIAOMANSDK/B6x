/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Main Entry of the application.
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

#define PWM_TMR_PSC            (16 - 1) // 16MHz
#define PWM_TMR_ARR            (100 - 1)

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

#if (DMA_USED)
#if (CTMR_USED)
#define PWM_CTMR_DMA_CHNL     (DMA_CH0)
#else
#define PWM_ATMR_DMA_CHNL     (DMA_CH1)
#endif
#define PWM_DUTY_CNT          (10) 

uint32_t pwm_duty_buff0[PWM_DUTY_CNT];
uint32_t pwm_duty_buff1[PWM_DUTY_CNT];

#define PA_DONE_SEE           (2)
#endif

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

static void pwmTest(void)
{
    #if (DMA_USED)
    uint16_t idx = 0;
    
    GPIO_DIR_SET_LO(1 << PA_DONE_SEE);
    dma_init();
    
    for (idx = 0; idx < PWM_DUTY_CNT; idx++)
    {
        pwm_duty_buff0[idx] = (idx + 1) * 10 - 1;
        pwm_duty_buff1[idx] = (PWM_DUTY_CNT - idx) * 10 - 1;
    }
    #endif
    
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
    // CTMR
    pwm_init(PWM_CTMR, PWM_TMR_PSC, PWM_TMR_ARR);

    chnl_conf.duty = 15;
    // duty/(arr+1) = 15%(Duty Ratio). 15% high, 85% low
    chnl_conf.ccer = CFG_PWM_CCER_SIPH;
    pwm_chnl_set(PWM_CTMR_CH1, &chnl_conf);

    chnl_conf.duty = 25; 
    // duty/(arr+1) = 25%(Duty Ratio). 25% low,  75% high
    chnl_conf.ccer = CFG_PWM_CCER_SIPL;
    pwm_chnl_set(PWM_CTMR_CH2, &chnl_conf);
    
    chnl_conf.duty = 35; 
    // duty/(arr+1) = 35%(Duty Ratio). 35% high, 65% low
    chnl_conf.ccer = CFG_PWM_CCER_SIPH;
    pwm_chnl_set(PWM_CTMR_CH3, &chnl_conf);

    chnl_conf.duty = 45; 
    // duty/(arr+1) = 45%(Duty Ratio). 45% low, 55% high
    chnl_conf.ccer = CFG_PWM_CCER_SIPL;
    pwm_chnl_set(PWM_CTMR_CH4, &chnl_conf);
    
    pwm_start(PWM_CTMR);
    #else
    // ADTMR
    pwm_init(PWM_ATMR, PWM_TMR_PSC, PWM_TMR_ARR);
    
    chnl_conf.duty = 10;
    chnl_conf.ccer = CFG_PWM_CCER_SIPH;
    // duty/(arr+1) = 10%(Duty Ratio). 10% high, 90% low
    pwm_chnl_set(PWM_ATMR_CH1P, &chnl_conf);
    // 10% low,  90% high
    pwm_chnl_set(PWM_ATMR_CH1N, &chnl_conf);
    
    chnl_conf.duty = 20;
    chnl_conf.ccer = CFG_PWM_CCER_SIPL;
    // duty/(arr+1) = 20%(Duty Ratio). 20% low,  80% high
    pwm_chnl_set(PWM_ATMR_CH2P, &chnl_conf);
    // 20% high, 80% low
    pwm_chnl_set(PWM_ATMR_CH2N, &chnl_conf);
    
    chnl_conf.duty = 30;
    chnl_conf.ccer = CFG_PWM_CCER_SIPH;
    // duty/(arr+1) = 30%(Duty Ratio). 30% high, 70% low
    pwm_chnl_set(PWM_ATMR_CH3P, &chnl_conf);
    // 30% low,  70% high
    pwm_chnl_set(PWM_ATMR_CH3N, &chnl_conf);
    
    chnl_conf.duty = 40;
    // duty/(arr+1) = 40%(Duty Ratio). 40% low, 60% high
    chnl_conf.ccer = CFG_PWM_CCER_SIPL;
    pwm_chnl_set(PWM_ATMR_CH4P, &chnl_conf);
    
    pwm_start(PWM_ATMR);
    #endif
    
    #if (DMA_USED)
    #if (CTMR_USED)
    DMA_CTMR_CHx_INIT(PWM_CTMR_DMA_CHNL, 1);
    DMA_CTMR_CHx_CONF(PWM_CTMR_DMA_CHNL, 1, pwm_duty_buff0, PWM_DUTY_CNT, CCM_PING_PONG);
    DMA_CTMR_CHx_CONF(PWM_CTMR_DMA_CHNL | DMA_CH_ALT, 1, pwm_duty_buff1, PWM_DUTY_CNT, CCM_PING_PONG);
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

static void sysInit(void)
{
    // Todo config, if need
    
}

static void devInit(void)
{
    iwdt_disable();
    
    dbgInit();
    debug("PWM Test...\r\n");
}

int main(void)
{
    sysInit();
    devInit();
    
    pwmTest();
    while (1);
}
