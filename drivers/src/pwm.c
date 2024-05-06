/**
 ****************************************************************************************
 *
 * @file pwm.c
 *
 * @brief Pulse Width Modulation(PWM) Driver
 *
 ****************************************************************************************
 */

#include "pwm.h"
#include "rcc.h"
#include "reg_timer.h"


/*
 * DEFINES
 ****************************************************************************************
 */

#if defined(CTMR_BASE)
#define TMR_USED(tmr)         ((TIMER_TypeDef* )(CTMR_BASE + (tmr) * 0x1000))
#else
#define TMR_USED(tmr)         (((tmr) == PWM_CTMR) ? CTMR : ATMR)
#endif


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

//void pwm_init(uint8_t tmr, const pwm_tmr_cfg_t *conf)
void pwm_init(uint8_t tmr, uint16_t psc, uint16_t arr)
{
    TIMER_TypeDef* TIMx = TMR_USED(tmr);
    uint32_t tmr_bit = 1UL << (1 + tmr); //APB_CTMR_BIT APB_ATMR_BIT
    
    //enable clock, init PSC & Freq
    RCC_APBCLK_EN(tmr_bit);
    TIMx->CR1.CEN = 0;
    
    //if (conf != NULL)
    {
        TIMx->CNT = 0;
        TIMx->PSC = psc; //conf->psc;
        TIMx->ARR = arr; //conf->arr;
    }

    TIMx->CR1.DIR = 0; //Counter used as upcounter
    TIMx->CR1.CMS = 0; //Edge-aligned mode
}

void pwm_conf(uint8_t tmr, uint16_t smcr, uint16_t intr)
{
    TIMER_TypeDef* TIMx = TMR_USED(tmr);
    
    TIMx->SMCR.Word = smcr;
    TIMx->IER.Word  = intr;
    TIMx->ICR.Word  = intr;
}

void pwm_start(uint8_t tmr)
{
    TIMER_TypeDef* TIMx = TMR_USED(tmr);
    
    if (tmr == PWM_ATMR)
    {
        TIMx->BDTR.MOE = 1;    
    }
    
    TIMx->CR1.ARPE = 1;
    TIMx->CR1.CEN = 1;
    TIMx->EGR.UG = 1;
}

#if (0)
void pwm_chnl_set(uint8_t chnl, const pwm_chnl_cfg_t *conf)
{
    uint8_t cidx = chnl % 4;
    TIMER_TypeDef* TIMx = TMR_USED(chnl / 4);
    
    // CCER&CCMR Clear
    TIMx->CCER.Word    &= ~(0x0F << (cidx * 4)); // bit[3:0]
    TIMx->DMAEN.Word   &= ~(0x01 << (cidx + 1)); // bit1 - CC1DE
    TIMx->CCMR[cidx/2] &= ~(0xFF << ((cidx % 2) * 8)); // bit[7:0]
    
    if (conf != NULL)
    {
        // CCR value
        TIMx->CCR[cidx]     = conf->duty;
        // CCMR conf
        TIMx->CCMR[cidx/2] |= (conf->ccmr << ((cidx % 2) * 8));
        // CCER enable
        TIMx->DMAEN.Word   |= (((conf->ccer & PWM_CCxDE_BIT) >> PWM_CCxDE_POS) << (cidx + 1));
        TIMx->CCER.Word    |= ((conf->ccer & 0x0F) << (cidx * 4));
    }   
}
#else

void pwm_chnl_set(uint8_t chnl, const pwm_chnl_cfg_t *conf)
{
    uint8_t used_nchnl = (chnl > PWM_ATMR_CH4P) ? 2 : 0;
    uint8_t cidx = chnl % 4;
    TIMER_TypeDef* TIMx = TMR_USED(chnl > PWM_CTMR_CH4);
    
    // CCER&CCMR Clear
//    TIMx->CCER.Word    &= ~(0x0F << (cidx * 4)); // bit[3:0]
    TIMx->CCER.Word    &= ~(0x03 << ((cidx * 4) + used_nchnl)); // p chnl --- bit[1:0], n - chnl --- bit[3:2]
    TIMx->DMAEN.Word   &= ~(0x01 << (cidx + 1)); // bit1 - CC1DE
//    TIMx->CCMR[cidx/2] &= ~(0xFF << ((cidx % 2) * 8)); // bit[7:0]

    if (conf != NULL)
    {
        // CCR value
        TIMx->CCR[cidx]     = conf->duty;
        // CCMR conf
        TIMx->CCMR[cidx/2] &= ~(0xFF << ((cidx % 2) * 8)); // bit[7:0]
        TIMx->CCMR[cidx/2] |= (conf->ccmr << ((cidx % 2) * 8));
        // CCER enable
        TIMx->DMAEN.Word   |= (((conf->ccer & PWM_CCxDE_BIT) >> PWM_CCxDE_POS) << (cidx + 1));
//        TIMx->CCER.Word    |= ((conf->ccer & 0x0F) << (cidx * 4));
        TIMx->CCER.Word    |= ((conf->ccer & 0x03) << ((cidx * 4) + used_nchnl));
    }
}
#endif

void pwm_duty_upd(uint8_t chnl, uint16_t duty)
{
    uint8_t cidx = chnl % 4;
    TIMER_TypeDef* TIMx = TMR_USED(chnl / 4);
    TIMx->CCR[cidx]     = duty;
}
