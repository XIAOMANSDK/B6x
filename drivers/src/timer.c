/**
 ****************************************************************************************
 *
 * @file timer.c
 *
 * @brief Base/Common/Advance Timer Driver
 *
 ****************************************************************************************
 */

#include "timer.h"
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

#if 0
void btmr_delay(uint16_t tpsc, uint16_t tcnt)
{
    // config Params
    BTMR->CR1.Word = CR1_ONE_MODE;
    BTMR->PSC = tpsc - 1;
    BTMR->ARR = tcnt - 1;
    BTMR->CNT = 0;
    BTMR->EGR = 1;
    
    // enable CEN, wait Time-Reach
    BTMR->CR1.CEN = 1;
    while(!BTMR->RIF && BTMR->CR1.CEN);
    BTMR->ICR = 1;
    
    // clear Mode
    BTMR->CR1.Word = 0;
}
#endif

void ctmr_init(uint16_t psc, uint16_t arr)
{
    RCC_APBCLK_EN(APB_CTMR_BIT);
    RCC_APBRST_REQ(APB_CTMR_BIT);
    
    CTMR->CR1.Word = 0; //.CEN = 0;
    CTMR->PSC = psc;
    CTMR->ARR = arr;
}

void ctmr_ctrl(uint16_t mode, uint16_t intr)
{
    // clear interrupt, then enable
    CTMR->ICR.Word = intr;
    CTMR->IER.Word = intr;
    // event generation
    CTMR->EGR.Word = intr | 0x01/*.UG*/;
    
    // conf control (counter start from 0)
    CTMR->CNT = 0;
    CTMR->CR1.Word = mode;
}

void adtmr_init(uint16_t psc, uint16_t arr)
{
    RCC_APBCLK_EN(APB_ATMR_BIT);
    RCC_APBRST_REQ(APB_ATMR_BIT);
    
    ATMR->CR1.Word = 0; // .CEN = 0;
    ATMR->PSC = psc;
    ATMR->ARR = arr;
}

void adtmr_ctrl(uint16_t mode, uint16_t intr)
{
    // clear interrupt, then enable
    ATMR->ICR.Word = intr;
    ATMR->IER.Word = intr;
    // event generation
    ATMR->EGR.Word = intr | 0x01/*.UG*/;
    
    // conf control (counter start from 0)
    ATMR->CNT = 0;
    ATMR->CR1.Word = mode;
}
