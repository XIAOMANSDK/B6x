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

#define IWDT_WINDOW        (32768)  //32.768KHz 1S 


/*
 * FUNCTIONS
 ****************************************************************************************
 */
volatile bool irq_evt = false;
volatile uint16_t irq_cnt = 0;

void IWDT_IRQHandler(void)
{
    iwdt_feed();
    irq_evt = true;
    irq_cnt++;
}

static void iwdtInit(void)
{
    iwdt_init(IWDT_INTEN_BIT | IWDT_CR_DFLT);
    iwdt_conf(IWDT_WINDOW);
    
    NVIC_EnableIRQ(IWDT_IRQn);
    __enable_irq();    
}

static void sysInit(void)
{
    // Todo config, if need
    
}

static void devInit(void)
{
    uint16_t rsn = rstrsn();
    
    dbgInit();

    debug("Start(rsn:0x%X)...\r\n", rsn);
    
    iwdtInit();    
}

int main(void)
{
    sysInit();
    devInit();
    
    while (1)
    {
        if (irq_evt)
        {
            irq_evt = false;
            debug("iwdt irq:%d\r\n", irq_cnt);
        }
    }
}
