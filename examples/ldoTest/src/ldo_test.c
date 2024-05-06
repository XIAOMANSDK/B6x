/**
 ****************************************************************************************
 *
 * @file ldo.c
 *
 * @brief LDO for Power on Reset, include BOD LVD.
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

#define GPIO_BOD            GPIO04
#define GPIO_LVD            GPIO05
#define GPIO_RUN            GPIO15

#define LDO_BOD_MSK         (LDO_BOD_ENB_MSK | LDO_BOD_TRIM_MSK)
#define LDO_BOD_CFG         (CFG_BOD_EN(1/*en*/, 1/*rst*/, 0/*irq*/) | CFG_BOD_TRIM(0))

#define LDO_LVD_MSK         (LDO_LVD_ENB_MSK | LDO_LVD_SEL_MSK)
#define LDO_LVD_CFG         (CFG_LVD_EN(1/*en*/, 1/*rst*/, 0/*irq*/) | CFG_LVD_SEL(7))


/*
 * FUNCTIONS
 ****************************************************************************************
 */

#if (BOD_TEST)
volatile bool bodFlg    = false;
volatile uint8_t bodCnt = 0;

void BOD12_IRQHandler(void)
{
    GPIO_DAT_SET(GPIO_BOD);
    
    bodFlg = true;
    bodCnt++;
    
    GPIO_DAT_CLR(GPIO_BOD);
}

static void bodEvent(void)
{
    if (bodFlg)
    {
        bodFlg = false;
        debug("BOD IRQ:%d\r\n", bodCnt);
    }
}
#endif

#if (LVD_TEST)
volatile bool lvdFlg = false;
volatile uint8_t lvdCnt = 0;

void LVD33_IRQHandler(void)
{
    GPIO_DAT_SET(GPIO_LVD);
    
    lvdFlg = true;
    lvdCnt++;
    
    GPIO_DAT_CLR(GPIO_LVD);
}

static void lvdEvent(void)
{
    if (lvdFlg)
    {
        lvdFlg = false;
        debug("LVD IRQ:%d\r\n", lvdCnt);
    }
}
#endif

static void ldoInit(void)
{
    uint32_t ldoCfg = core_ldoget();
    debug("LDO Get:0x%X\r\n", ldoCfg);
    
    #if (BOD_TEST)
    // extern 1.2V 
    APBMISC->AON_PMU_CTRL.AON_PWR_SEL_RUN = 0;
    AON->PMU_CTRL.CORELDO_EN_RUN = 0;
    
    ldoCfg = (ldoCfg & ~LDO_BOD_MSK) | LDO_BOD_CFG;
    NVIC_EnableIRQ(BOD12_IRQn);
    #endif
    
    #if (LVD_TEST)
    ldoCfg = (ldoCfg & ~LDO_LVD_MSK) | LDO_LVD_CFG;
    NVIC_EnableIRQ(LVD33_IRQn);
    #endif
    
    core_ldoset(ldoCfg);
    bootDelayMs(15); // must wait
    
    debug("LDO Set:0x%X\r\n", ldoCfg);
}

void ldoTest(void)
{
    GPIO_DIR_SET_LO(GPIO_BOD | GPIO_LVD | GPIO_RUN);
    
    // init BOD LVD
    GPIO_DAT_SET(GPIO_RUN);
    ldoInit();
    GPIO_DAT_CLR(GPIO_RUN);

    __enable_irq();

    while (1)
    {
        // event proc
        #if (BOD_TEST)
        bodEvent();
        #endif
        
        #if (LVD_TEST)
        lvdEvent();
        #endif
        
        // heart pulse
        GPIO_DAT_SET(GPIO_RUN);
        bootDelayMs(2);
        GPIO_DAT_CLR(GPIO_RUN);
    }
}
