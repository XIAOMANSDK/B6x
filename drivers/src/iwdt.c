/**
 ****************************************************************************************
 *
 * @file iwdt.c
 *
 * @brief WatchDog Timer Driver
 *
 ****************************************************************************************
 */

#include "iwdt.h"
#include "rcc.h"
#include "reg_iwdt.h"


/*
 * DEFINES
 ****************************************************************************************
 */



/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void iwdt_init(uint8_t ctrl)
{
    RCC_APBCLK_EN(APB_IWDT_BIT);

    IWDT->LOCK    = 0x1ACCE551; // unlock iwdt
    IWDT->CTRL.EN = 0;
    IWDT->INTCLR  = 1; // clear int

    IWDT->CTRL.Word = ctrl;
    IWDT->LOCK = 0; // lock iwdt
}

void iwdt_feed(void)
{
    IWDT->LOCK   = 0x1ACCE551; // unlock iwdt
    IWDT->INTCLR = 1; // clear to reload
    IWDT->LOCK   = 0; // lock iwdt
}

void iwdt_deinit(void)
{
    IWDT->LOCK      = 0x1ACCE551; // unlock iwdt
    IWDT->INTCLR    = 1; // clear int
    IWDT->CTRL.Word = 0x08;
    IWDT->LOCK      = 0; // lock iwdt
    RCC_APBCLK_DIS(APB_IWDT_BIT);
}

#if (ROM_UNUSED)
uint32_t iwdt_conf(uint32_t load) // add 6vp 1125
{
    uint32_t value;

    IWDT->LOCK = 0x1ACCE551; // unlock iwdt
    IWDT->INTCLR = 1; // clear int

    value = IWDT->VALUE;
    if (load > 0)
    {
        IWDT->LOAD = load; // feed dog
        IWDT->CTRL.EN = 1;
    }
    else
    {
        IWDT->CTRL.EN = 0; // disable dog
    }

    IWDT->LOCK = 0; // lock iwdt

    return value;
}
#endif // (ROM_UNUSED)
