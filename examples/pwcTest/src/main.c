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
#include "regs.h"
#include "dbg.h"


/*
 * DEFINES
 ****************************************************************************************
 */

#define PA_RET_SEE1           (9)

#define POS_NEG_EDGE          (3)

#if (POS_NEG_EDGE == 1)
#define PA_TMR_EDGE           (PWC_CCER_POSEDGE)
#define PWC_CC1S              (1)
#define PWC_SMCR_TS_SMS       (0x54)// start with SMCR.TS=5(TI1FP1), SMCR.SMS=4(reset mode)
#elif (POS_NEG_EDGE == 2)
#define PA_TMR_EDGE           (PWC_CCER_NEGEDGE)
#define PWC_CC1S              (1)
#define PWC_SMCR_TS_SMS       (0x54)// start with SMCR.TS=5(TI1FP1), SMCR.SMS=4(reset mode)
#elif (POS_NEG_EDGE == 3)
// only ch1 support bilateral.(NEGEDGE & POSEDGE)
#define PA_TMR_EDGE           (PWC_CCER_POSEDGE | PWC_CCER_NEGEDGE)
#define PWC_CC1S              (3)
#define PWC_SMCR_TS_SMS       (0x44)// start with SMCR.TS=4(TI1F_ED), SMCR.SMS=4(reset mode)
#endif

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


#define TIMER_INT_CH1_BIT     (0x02U)
#define TIMER_INT_CH2_BIT     (0x04U)
#define TIMER_INT_CH3_BIT     (0x08U)
#define TIMER_INT_CH4_BIT     (0x10U)
/*
 * FUNCTIONS
 ****************************************************************************************
 */

#if (CTMR_USED)
void CTMR_IRQHandler(void)
{
    uint32_t iflg = CTMR->IFM.Word;
    
    if (iflg & TIMER_INT_CH1_BIT) // ch1 interrupt
    {
        GPIO_DAT_SET(1 << PA_RET_SEE1);
        debug("TIME:[%d ms] LEVEL:[%d]\r\n", CTMR->CCR1+1, gpio_get(PA_PWC_CH1) ^ 0x01);
        CTMR->ICR.CC1I = 1;
        GPIO_DAT_CLR(1 << PA_RET_SEE1);
    }
}
#else
void ATMR_IRQHandler(void)
{
    uint32_t iflg = ATMR->IFM.Word;
    
    if (iflg & TIMER_INT_CH1_BIT) // ch1 interrupt
    {
        GPIO_DAT_SET(1 << PA_RET_SEE1);
        ATMR->ICR.CC1I = 1;
        GPIO_DAT_CLR(1 << PA_RET_SEE1);
    }
}
#endif

static void pwcTest(void)
{
    // init PADs
    GPIO_DAT_CLR((1 << PA_RET_SEE1)| (1 << PA_PWC_CH1));
    GPIO_DIR_SET((1 << PA_RET_SEE1));
    GPIO_DIR_CLR((1 << PA_PWC_CH1));
    
    #if (CTMR_USED)
        // PA0~PA19
        iom_ctrl(PA_PWC_CH1, IOM_PULLDOWN | IOM_INPUT | IOM_SEL_CSC);
        csc_input(PA_PWC_CH1, CSC_CTMR_CH1);
    #else    
        iom_ctrl(PA_PWC_CH1, IOM_PULLDOWN | IOM_INPUT | IOM_SEL_TIMER);
    #endif
    // init pwm timer
    pwm_init(PWC_TMR, 15999, UINT16_MAX);  // 1ms
    
    // pwc_chnl config
    pwm_chnl_cfg_t chnl_cfg;
    
    chnl_cfg.duty = 0;
    chnl_cfg.ccmr = PWC_CCMR_MODE(PWC_CC1S, 3, PWC_PSC0);
    
    #if (POS_NEG_EDGE != 3)
        chnl_cfg.ccer = PA_TMR_EDGE;
    #endif

    pwm_chnl_set(PWC_TMR_CH(1), &chnl_cfg);

    pwm_conf(PWC_TMR, PWC_SMCR_TS_SMS, TIMER_INT_CH1_BIT | TIMER_INT_CH2_BIT); 
    pwm_start(PWC_TMR);
    
    // interrupt enable 
    NVIC_EnableIRQ(PWC_IRQc); 
    __enable_irq();
}

static void sysInit(void)
{
    // Todo config, if need
    
}

static void devInit(void)
{
    iwdt_disable();
    
    dbgInit();
    debug("PWC Test...\r\n"); 
}

int main(void)
{
    sysInit();
    devInit();
    
    pwcTest();
    
    while (1);
}
