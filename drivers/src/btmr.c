#include "btmr.h"
#include "reg_rcc.h"


#define CR1_ONE_MODE           (0x0C) // one-time mode .URS=1(bit2), .OPM=1(bit3)


// timeout = arr * 1ms
void timeOutMsInit(uint16_t arr)
{
    RCC->APBCLK_EN_RUN.BSTIM1_CLKEN_RUN = 1;
    BTMR->CR1.CEN = 0; // Add Disable BSTIM
    BTMR->PSC = 15999;  //
    BTMR->ARR = (uint32_t)arr;
    BTMR->CR1.OPM = 1;
    BTMR->CR1.ARPE = 1;
    BTMR->EGR = 1;
    BTMR->CR1.CEN = 1;
    BTMR->ICR = 1;
}


#if 0
void btmrConfig(void)
{
    RCC->APBCLK_EN_RUN.BSTIM1_CLKEN_RUN = 1;

    BTMR->CR1.Word = 0;
    //BTMR->PSC = 15;
    BTMR->ARR = 65535;
    // update enable
    BTMR->CR1.URS = 1;
    BTMR->CR1.OPM = 1;
    //BTMR->CR1.UDIS = 1;
    // interrupt enable
    //BTMR->IER.Word = 1;
    //BTMR->ICR.Word = 1;
    //__enable_irq();
    //NVIC_EnableIRQ(BTMR_IRQn);
}

#if 0
uint32_t ustime = 0;
void BTMR_IRQHandler(void)
{
    // clear interrupt
    BTMR->IDR.Word = 1;
    BTMR->ICR.Word = 1;
    if (ustime > 0)
        ustime--;
    BTMR->IER.Word = 1;
}
#endif
void bootDelayUs(uint32_t us)
{
    BTMR->PSC = 15;
    BTMR->EGR = 1;
    BTMR->CNT = 65535-us+1;
    BTMR->CR1.CEN = 1;
    while(!BTMR->RIF);
    BTMR->ICR = 1;
    BTMR->CR1.CEN = 0;
}

void bootDelayMs(uint32_t ms)
{
    BTMR->PSC = 15999;
    BTMR->EGR = 1;
    BTMR->CNT = 65535-ms+1;
    BTMR->CR1.CEN = 1;
    while(!BTMR->RIF);
    BTMR->ICR = 1;
    BTMR->CR1.CEN = 0;
}

#endif

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

#include "b6x.h"

void tick_delay(uint16_t tpsc, uint16_t tcnt)
{
    uint32_t temp;

    SysTick->LOAD = tcnt * tpsc - 1;
    SysTick->VAL  = 0x00;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;

    do
    {
        temp = SysTick->CTRL;
    } while((temp & SysTick_CTRL_ENABLE_Msk) && !(temp & SysTick_CTRL_COUNTFLAG_Msk));

    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
    SysTick->VAL   = 0x00;
}
