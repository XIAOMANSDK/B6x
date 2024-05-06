/**
 ****************************************************************************************
 *
 * @file rco_test.c
 *
 * @brief RC32K/RC16M clkout and calib.
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

#define GPIO_32K            GPIO11
#define GPIO_16M            GPIO12
#define GPIO_RUN            GPIO13

#define RC32K_REF_CLK       RCLK_DPLL
#define RC32K_CAL_CTL       RCAL_CYCLES(4)
#define RC32K_CAL_NB        8
#define RC32K_CAL_DIFF      2

/*
 * FUNCTIONS
 ****************************************************************************************
 */

#if (RC32K_TEST)

uint16_t rc32kCal;

static __forceinline int co_abs(int val)
{
    return val < 0 ? val*(-1) : val;
}

static void rc32kInit(void)
{
    // clk watch, rc32k calib
    iospc_clkout(CLK_OUT_LSI);
    
    rc32k_conf(RC32K_REF_CLK, RC32K_CAL_CTL);
    
    for (uint8_t i = 0; i < RC32K_CAL_NB; i++)
    {
        GPIO_DAT_SET(GPIO_32K);
        rc32kCal = rc32k_calib();
        GPIO_DAT_CLR(GPIO_32K);
        
        debug("RC32K Cal%d(msb:0x%X,lsb:0x%X)\r\n", i, rc32kCal&0xF, rc32kCal>>4);
    }
}

static bool rc32kChgd(void)
{
    uint16_t curcal;
    
    GPIO_DAT_SET(GPIO_32K);
    curcal = rc32k_calib();
    GPIO_DAT_CLR(GPIO_32K);
    
    if (((curcal&0xF) != (rc32kCal&0xF))/*MSB*/
        || (co_abs((curcal>>4)-(rc32kCal>>4)) > RC32K_CAL_DIFF)/*LSB*/)
    {
        debug("RC32K Changed(msb:0x%X,lsb:0x%X)\r\n", rc32kCal&0xF, rc32kCal>>4);
        return true;
    }
    return false;
}
#endif

#if (RC16M_TEST)

uint8_t rc16mCal;

static void rc16mInit(void)
{
    uint8_t curtrim;
    
    iospc_clkout(CLK_OUT_HSI);
    
    curtrim = rc16m_trim_get();
    GPIO_DAT_SET(GPIO_16M);
    rc16mCal = rc16m_calib();
    GPIO_DAT_CLR(GPIO_16M);
    
    debug("RC16M Cal(%d->%d)\r\n", curtrim, rc16mCal);
}

static bool rc16mChgd(void)
{
    uint8_t curcal;
    
    GPIO_DAT_SET(GPIO_16M);
    curcal = rc16m_calib();
    GPIO_DAT_CLR(GPIO_16M);
    
    if (curcal != rc16mCal)
    {
        debug("RC16M Changed(%d->%d)\r\n", rc16mCal, curcal);
        rc16mCal = curcal;
        return true;
    }
    return false;
}
#endif

void rcoTest(void)
{
    bool chgd = false;
    rtc_time_t time;
    
    GPIO_DIR_SET_LO(GPIO_32K | GPIO_16M | GPIO_RUN);
    
    #if (RC16M_TEST)
    rc16mInit();
    #endif

    #if (RC32K_TEST)
    rc32kInit();
    #endif
    
    rtc_conf(true);
    time = rtc_time_get();
    debug("RTC Time:%d.%03d\r\n", time.sec, time.ms);

    while (1)
    {
        GPIO_DAT_SET(GPIO_RUN);
        #if (RC16M_TEST)
        if (rc16mChgd()) chgd = true;
        #endif
        
        #if (RC32K_TEST)
        if (rc32kChgd()) chgd = true;
        #endif
    
        if (chgd)
        {
            chgd = false;
            
            time = rtc_time_get();
            debug("at RTC Time:%d.%03d\r\n", time.sec, time.ms);
        }
        
        GPIO_DAT_CLR(GPIO_RUN);
    }
}
