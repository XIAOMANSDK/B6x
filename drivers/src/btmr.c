#include "b6x.h"
#include "reg_btmr.h"
#include "btmr.h"

#if (ROM_UNUSED)
#define CR1_ONE_MODE           (0x0C) // one-time mode .URS=1(bit2), .OPM=1(bit3)

void btmr_delay(uint16_t tpsc, uint16_t tcnt)
{
    // config Params
    BTMR->CR1.Word = CR1_ONE_MODE;
    BTMR->PSC = tpsc - 1;
    BTMR->ARR = tcnt - 1;
    BTMR->CNT = 0;
    BTMR->EGR = 1;
    BTMR->ICR = 1;

    // enable CEN, wait Time-Reach
    BTMR->CR1.CEN = 1;
    while(!BTMR->RIF && BTMR->CR1.CEN);
    BTMR->ICR = 1;

    // clear Mode
    BTMR->CR1.Word = 0;
}
#endif

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
