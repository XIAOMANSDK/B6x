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

#define PA_RET_SEE          (15)
#define PA_EXTI0            (16)
#define PA_EXTI1            (17)


/*
 * FUNCTIONS
 ****************************************************************************************
 */

volatile uint8_t gExtiIrqFlag = 0;

void EXTI_IRQHandler(void)
{
    uint32_t irq_sta = EXTI->RIF.Word;
    
    gExtiIrqFlag = 0;    
    
    //GPIO_DAT_SET(1 << PA_RET_SEE);
    gpio_put_hi(PA_RET_SEE);
    
    if (irq_sta & EXTI_SRC(PA_EXTI0))
    {
        //EXTI->IDR.Word = EXTI_SRC(PA_EXTI0));
        EXTI->ICR.Word = EXTI_SRC(PA_EXTI0);
        gExtiIrqFlag |= 0x01;
        //EXTI->IER.Word = EXTI_SRC(PA_EXTI0);
    }
    
    if (irq_sta & EXTI_SRC(PA_EXTI1))
    {
        //EXTI->IDR.Word = EXTI_SRC(PA_EXTI1);
        EXTI->ICR.Word = EXTI_SRC(PA_EXTI1);
        gExtiIrqFlag |= 0x02;
        //EXTI->IER.Word = EXTI_SRC(PA_EXTI1);
    }
    
    //GPIO_DAT_CLR(1 << PA_RET_SEE);
    gpio_put_lo(PA_RET_SEE);
}

static void extiTest(void)
{
    // IO config
    gpio_dir_output(PA_RET_SEE, OE_LOW);
    
    gpio_dir_input(PA_EXTI0, IE_UP);
    gpio_dir_input(PA_EXTI1, IE_DOWN);
    
    // EXTI config
    exti_init(EXTI_DBC(15, 4));
    exti_set(EXTI_FTS, EXTI_SRC(PA_EXTI0)); // falling
    exti_set(EXTI_RTS, EXTI_SRC(PA_EXTI1)); // rising
    exti_set(EXTI_DBE, EXTI_SRC(PA_EXTI0) | EXTI_SRC(PA_EXTI1));
    exti_set(EXTI_IER, EXTI_SRC(PA_EXTI0) | EXTI_SRC(PA_EXTI1));
    
    // IRQ enable
    NVIC_EnableIRQ(EXTI_IRQn);
    __enable_irq();
    
    while (1)
    {
        if (gExtiIrqFlag)
        {
            debug("Trig: %X\r\n", gExtiIrqFlag);
            gExtiIrqFlag = 0;
        }
    }
}

static void sysInit(void)
{
    // Todo config, if need
    
}

static void devInit(void)
{
    iwdt_disable();
    
    dbgInit();
    debug("EXTI Test...\r\n"); 
}

int main(void)
{
    sysInit();
    devInit();
    
    extiTest();
    while (1);
}
