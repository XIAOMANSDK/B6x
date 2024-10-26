/**
 ****************************************************************************************
 *
 * @file exti.c
 *
 * @brief EXTI Config
 *
 ****************************************************************************************
 */

#include "exti.h"
#include "rcc.h"
#include "reg_exti.h"

void exti_init(uint16_t debounce)
{
    RCC_APBCLK_EN(APB_EXTI_BIT);
    RCC_APBRST_REQ(APB_EXTI_BIT);

    EXTI->DBC.Word = debounce;
}

void exti_deinit(void)
{
    RCC_APBCLK_DIS(APB_EXTI_BIT);
    RCC_APBRST_REQ(APB_EXTI_BIT);
}

void exti_set(uint8_t typ, uint32_t loca)
{
    (*(volatile uint32_t*)(EXTI_BASE + typ * 4)) = loca;
}

uint32_t exti_get(uint8_t typ)
{
    return (*(volatile uint32_t*)(EXTI_BASE + typ * 4));
}
