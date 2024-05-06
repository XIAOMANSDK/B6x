/**
 ****************************************************************************************
 *
 * @file rtc.h
 *
 * @brief Header file - RTC Driver
 *
 ****************************************************************************************
 */

#ifndef _RTC_H_
#define _RTC_H_

#include <stdint.h>
#include <stdbool.h>
#include "reg_apbmisc.h"


/*
 * DEFINES
 ****************************************************************************************
 */

/// Time information
typedef struct {
    // Second part of the time
    uint32_t sec;
    // Millisecond part of the time (range: 0~999ms)
    uint32_t ms;
} rtc_time_t;


/*
 * MACROS DECLARATION
 ****************************************************************************************
 */

/// Add 'ms/1000' into 'sec', 'ms' is 0~999
#define SEC_MS_ADD(sec, ms)     while (ms >= 1000) { ms -= 1000; sec++; }


/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Config rtc module, enabled after 1ms if set True.
 *
 * @param[in] en  True to enable, False to disable
 * 
 ****************************************************************************************
 */
void rtc_conf(bool en);

/**
 ****************************************************************************************
 * @brief Get current rtc-time.
 *
 * @return rtc-time value
 ****************************************************************************************
 */
rtc_time_t rtc_time_get(void);

/**
 ****************************************************************************************
 * @brief Set/Change current time
 *
 * @param[in] sec  second time
 * @param[in] ms   millisecond time (range: 0~999ms)
 * 
 ****************************************************************************************
 */
void rtc_time_set(uint32_t sec, uint32_t ms);

/**
 ****************************************************************************************
 * @brief Renew alarm time, used in RTC_IRQHandler() for periodic calls
 *
 * @param[in] ms_time  time(unit in ms) to alarm again
 * 
 ****************************************************************************************
 */
static inline void rtc_alarm_renew(uint32_t ms_time)
{
    //APBMISC->RTCINT_CTRL.RTC_INT_EN = 0;
    APBMISC->RTCINT_CTRL.RTC_INT_CLR = 1;

    //if (!APBMISC->RTCINT_CTRL.RTC_SET_BUSY)
    {
        // Read current alarm time, add ms_time to renew it. 
        uint32_t alm_sec = APBMISC->RTC_ALARM_SEC;
        uint32_t alm_ms  = APBMISC->RTC_ALARM_MS + ms_time;

        SEC_MS_ADD(alm_sec, alm_ms);
        APBMISC->RTC_ALARM_SEC = alm_sec;
        APBMISC->RTC_ALARM_MS  = alm_ms;
    }

    //APBMISC->RTCINT_CTRL.RTC_INT_EN = 1;
}

/**
 ****************************************************************************************
 * @brief Set alarm time
 *
 * @param[in] ms_time  time(unit in ms) to alarm
 * 
 ****************************************************************************************
 */
void rtc_alarm_set(uint32_t ms_time);

/**
 ****************************************************************************************
 * @brief Get configured alarm rtc-time.
 *
 * @return rtc-time value
 ****************************************************************************************
 */
rtc_time_t rtc_alarm_get(void);

/**
 ****************************************************************************************
 * @brief Judge alarm time reached or not, auto clear if reached.
 *
 * @return True if time reached, else False 
 ****************************************************************************************
 */
bool rtc_is_alarm(void);

/**
 ****************************************************************************************
 * @brief Set rtc interrupt mode
 *
 * @param[in] en  True to interrupt enable, False to disable
 * 
 ****************************************************************************************
 */
void rtc_irq_set(bool en);

/**
 ****************************************************************************************
 * @brief Set rtc wakeup mode
 *
 * @param[in] en  True to enable wakeup, False to disable
 * 
 ****************************************************************************************
 */
void rtc_wkup_set(bool en);


#endif // _RTC_H_
