/**
 ****************************************************************************************
 *
 * @file rtc.c
 *
 * @brief Real Time Clock(RTC) Driver
 *
 ****************************************************************************************
 */

#include "rtc.h"
#include "reg_aon.h"
#include "reg_apbmisc.h"


/*
 * DEFINES
 ****************************************************************************************
 */



/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Config rtc module
 *
 * @param[in] en  True to enable, False to disable
 *
 ****************************************************************************************
 */
void rtc_conf(bool en)
{
    #if (ROM_UNUSED)
    // clock enable (keep enabled in rom)
    // RCC->APBCLK_EN_RUN.AON_CLKEN_RUN     = 1;
    // RCC->APBCLK_EN_RUN.APBMISC_CLKEN_RUN = 1;
    RCC_APBCLK_EN(APB_APBMISC_BIT | APB_AON_BIT);
    #endif

    // rtc interrupt disable and status clear
    APBMISC->RTCINT_CTRL.Word = 0x02; // .RTC_INT_EN = 0, .RTC_INT_CLR = 1

    // rtc enable or disable
    AON->BLE_RTC_CTL.RTC_EN = en;
}

/**
 ****************************************************************************************
 * @brief Get current rtc-time.
 *
 * @return rtc-time value
 ****************************************************************************************
 */
rtc_time_t rtc_time_get(void)
{
    rtc_time_t time;

    time.sec = APBMISC->RTC_SEC_SHD;
    time.ms  = APBMISC->RTC_MS_SHD;

    if (time.sec != APBMISC->RTC_SEC_SHD)
    {
        // just past to next second
        time.sec = APBMISC->RTC_SEC_SHD;
        time.ms  = APBMISC->RTC_MS_SHD;
    }

    return time;
}

/**
 ****************************************************************************************
 * @brief Set/Change current time
 *
 * @param[in] sec  second time
 * @param[in] ms   millisecond time (range: 0~999ms)
 *
 ****************************************************************************************
 */
void rtc_time_set(uint32_t sec, uint32_t ms)
{
    APBMISC->RTC_SEC = sec;
    APBMISC->RTC_MS  = ms;

    // wait rtc current time reg write finish
    while(APBMISC->RTCINT_CTRL.RTC_SET_BUSY);
}

/**
 ****************************************************************************************
 * @brief Set alarm time
 *
 * @param[in] ms_time  time(unit in ms) to alarm
 *
 ****************************************************************************************
 */
void rtc_alarm_set(uint32_t ms_time)
{
    // Disable Interrupt, clear last
    APBMISC->RTCINT_CTRL.RTC_INT_EN = 0;
    APBMISC->RTCINT_CTRL.RTC_INT_CLR = 1;

    if (ms_time)
    {
        rtc_time_t time;

        // Wait rtc alarm time reg write finish
        while(APBMISC->RTCINT_CTRL.RTC_SET_BUSY);

        // time = rtc_time_get();
        time.sec = APBMISC->RTC_SEC_SHD;
        time.ms  = APBMISC->RTC_MS_SHD;

        if (time.sec != APBMISC->RTC_SEC_SHD)
        {
            // just past to next second
            time.sec = APBMISC->RTC_SEC_SHD;
            time.ms  = APBMISC->RTC_MS_SHD;
        }

        // Add time, then set alarm
        ms_time += time.ms;
        APBMISC->RTC_ALARM_SEC = time.sec + (ms_time / 1000);
        APBMISC->RTC_ALARM_MS  = ms_time % 1000;

        // Enable Interrupt
        APBMISC->RTCINT_CTRL.RTC_INT_EN = 1;
    }
}

/**
 ****************************************************************************************
 * @brief Get configured alarm rtc-time.
 *
 * @return rtc-time value
 ****************************************************************************************
 */
rtc_time_t rtc_alarm_get(void)
{
    rtc_time_t time;

    time.sec = APBMISC->RTC_ALARM_SEC;
    time.ms  = APBMISC->RTC_ALARM_MS;
    return time;
}

/**
 ****************************************************************************************
 * @brief Judge alarm time reached or not, auto clear if reached.
 *
 * @return True if time reached, else False
 ****************************************************************************************
 */
bool rtc_is_alarm(void)
{
    bool ret = APBMISC->RTCINT_CTRL.RTC_INT_ST;

    if (ret)
    {
        APBMISC->RTCINT_CTRL.RTC_INT_CLR = 1;
    }

    return ret;
}

/**
 ****************************************************************************************
 * @brief Set rtc interrupt mode
 *
 * @param[in] en  True to interrupt enable, False to disable
 *
 ****************************************************************************************
 */
void rtc_irq_set(bool en)
{
    APBMISC->RTCINT_CTRL.RTC_INT_EN = en;
    APBMISC->RTCINT_CTRL.RTC_INT_CLR = 1;
}

/**
 ****************************************************************************************
 * @brief Set rtc wakeup mode
 *
 * @param[in] en  True to enable wakeup, False to disable
 *
 ****************************************************************************************
 */
void rtc_wkup_set(bool en)
{
    AON->PMU_WKUP_CTRL.RTC_WKUP_EN = en;
    APBMISC->AON_PMU_CTRL.WKUP_ST_CLR = 1;
}
