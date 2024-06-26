/**
 ****************************************************************************************
 *
 * @file rtc_test.c
 *
 * @brief RTC Time Alarm to ADTMR compare.
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

#define GPIO_RTC            GPIO11
#define GPIO_TMR            GPIO12
#define GPIO_RUN            GPIO13

#define RTC_ALARM_TIME      1000 // unit in ms

#define RC32K_CAL_NB        8

#define TMR_PSC             (48 - 1) // 1us sysclk=(n)MHz
#define TMR_ARR             0xFFFF


/*
 * FUNCTIONS
 ****************************************************************************************
 */

volatile bool rtcFlg;
volatile uint16_t tmrCnt;
volatile uint32_t tmrVal;

void RTC_IRQHandler(void)
{
    GPIO_DAT_SET(GPIO_RTC);
    
    rtcFlg = true;
    
    // Once mode - disable
    //rtc_irq_set(0); or rtc_alarm_set(0);
    
    // Period mode - reload
    rtc_alarm_set(RTC_ALARM_TIME);
    
    // Record ADTMR value to compare
    tmrVal = ATMR->CNT + ((uint32_t)tmrCnt << 16); // unit in us
    // the counter start from 0
    ATMR->CNT = 0; 
    tmrCnt = 0;

    GPIO_DAT_CLR(GPIO_RTC);
}

void ADMR1_IRQHandler(void)
{
    GPIO_DAT_SET(GPIO_TMR);
    
    uint32_t irq_stat = ATMR->RIF.Word;
    
    // Each  tick Interrupt is 10ms
    if (irq_stat & 0x01/*TIMER_UI_BIT*/)
    {
        ATMR->IDR.UI = 1;  // Disable UI Interrupt
        
        tmrCnt++;
        
        ATMR->ICR.UI = 1; //Clear Interrupt Flag
        ATMR->IER.UI = 1; // Enable UI Interrupt
    }
    
    GPIO_DAT_CLR(GPIO_TMR);
}

void rtcTest(void)
{
    uint16_t rc32cal;
    rtc_time_t time, alarm;
    
    GPIO_DIR_SET_LO(GPIO_RTC | GPIO_TMR | GPIO_RUN);
    
    // clk watch, rc32k calib
    iospc_clkout(CLK_OUT_LSI);
    
    rc32k_conf(RCLK_HSE, RCAL_CYCLES(0x1F));
    for (uint8_t i = 0; i < RC32K_CAL_NB; i++)
    {
        GPIO_DAT_SET(GPIO_RUN);
        rc32cal = rc32k_calib();
        GPIO_DAT_CLR(GPIO_RUN);
        
        debug("RC32K Cal%d:0x%X\r\n", i, rc32cal);
    }
    
    // rtc enable, set alarm
    rtc_conf(true);
    rtc_alarm_set(RTC_ALARM_TIME);
    NVIC_EnableIRQ(RTC_IRQn);

    // tmr enable
    atmr_init(TMR_PSC, TMR_ARR);
    atmr_ctrl(TMR_PERIOD_MODE, TMR_IR_UI_BIT);
    NVIC_EnableIRQ(ATMR_IRQn);

    __enable_irq();

    time = rtc_time_get();
    alarm = rtc_alarm_get();
    debug("0-RTC(Time:%d.%03d,Alarm:%d.%03d)\r\n", time.sec, time.ms, alarm.sec, alarm.ms);
    
    while (1)
    {
        GPIO_DAT_SET(GPIO_RUN);
        
        if (rtcFlg)
        {
            rtcFlg = false;
            debug("TMR(Cnt:%03d,Val:0x%X)\r\n", tmrCnt, tmrVal);
            
            time = rtc_time_get();
            alarm = rtc_alarm_get();
            debug("1-RTC(Time:%d.%03d,Alarm:%d.%03d)\r\n", time.sec, time.ms, alarm.sec, alarm.ms);
        }
        
        GPIO_DAT_CLR(GPIO_RUN);
    }
}
