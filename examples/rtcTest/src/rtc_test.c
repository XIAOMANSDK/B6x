/**
 ****************************************************************************************
 *
 * @file rtc_test.c
 *
 * @brief RTC alarm and ATMR comparison demo.
 *
 * @details
 * Demonstrates RTC (Real-Time Clock) basic usage:
 * - Configure RTC module and set initial time
 * - Set periodic alarm, reload in interrupt
 * - Use advanced timer (ATMR) to record alarm trigger time
 * - Output status via GPIO for timing accuracy verification
 *
 * Operation:
 * 1. RTC config: enable RTC, set seconds and milliseconds counters
 * 2. Alarm setup: configure alarm time, single-shot or periodic mode
 * 3. Interrupt: reload alarm and record ATMR time on trigger
 * 4. Time comparison: verify RTC alarm via ATMR microsecond precision
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

// GPIO pins for status indication
#define GPIO_RTC            GPIO11  ///< RTC interrupt status
#define GPIO_TMR            GPIO12  ///< ATMR interrupt status
#define GPIO_RUN            GPIO13  ///< Main loop running status

// RTC alarm parameters
#define RTC_ALARM_TIME      1000    ///< Alarm period: 1000ms (1 second)
#define RC32K_CAL_NB        8       ///< RC32K calibration iterations

// Advanced timer parameters
#define TMR_PSC             (16 - 1) // 1us tick @ 16MHz sysclk
#define TMR_ARR             0xFFFF   // ATMR auto-reload: 16-bit max

/*
 * GLOBAL VARIABLES
 ****************************************************************************************
 */

static volatile bool rtcFlg;        ///< RTC interrupt event flag
static volatile uint16_t tmrCnt;    ///< ATMR interrupt counter (10ms ticks)
static volatile uint32_t tmrVal;    ///< ATMR capture value on alarm trigger (us)

/*
 * FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief RTC interrupt service routine.
 *
 * @details Handles RTC alarm interrupt:
 *          set flag, reload alarm, capture ATMR value, reset counters.
 ****************************************************************************************
 */
void RTC_IRQHandler(void)
{
    GPIO_DAT_SET(GPIO_RTC);

    rtcFlg = true;

    // Periodic mode: reload alarm
    rtc_alarm_set(RTC_ALARM_TIME);

    // Capture ATMR counter value (combined with tmrCnt for extended range)
    tmrVal = ATMR->CNT + ((uint32_t)tmrCnt << 16);

    // Reset ATMR counters for next period measurement
    ATMR->CNT = 0;
    tmrCnt = 0;

    GPIO_DAT_CLR(GPIO_RTC);
}

/**
 ****************************************************************************************
 * @brief ATMR interrupt service routine.
 *
 * @details Handles ATMR update interrupt (~10ms period):
 *          check UI flag, increment counter.
 ****************************************************************************************
 */
void ATMR_IRQHandler(void)
{
    GPIO_DAT_SET(GPIO_TMR);

    uint32_t irq_stat = ATMR->RIF.Word;

    if (irq_stat & TMR_IR_UI_BIT)
    {
        ATMR->IDR.UI = 1;
        tmrCnt++;
        ATMR->ICR.UI = 1;
        ATMR->IER.UI = 1;
    }

    GPIO_DAT_CLR(GPIO_TMR);
}

/**
 ****************************************************************************************
 * @brief RTC test main function.
 *
 * @details Complete RTC test flow:
 *          GPIO init, RC32K calibration, RTC config, ATMR config, main loop.
 ****************************************************************************************
 */
void rtcTest(void)
{
    uint16_t rc32cal;
    rtc_time_t time, alarm;

    // Configure status indicator GPIOs as output
    GPIO_DIR_SET_LO(GPIO_RTC | GPIO_TMR | GPIO_RUN);

    // Output LSI clock for frequency observation
    iospc_clkout(CLK_OUT_LSI);

    // RC32K calibration configuration
    rc32k_conf(RCLK_HSE, RCAL_CYCLES(0x1F));

    // Perform multiple calibration iterations for stability
    for (uint8_t i = 0; i < RC32K_CAL_NB; i++)
    {
        GPIO_DAT_SET(GPIO_RUN);
        rc32cal = rc32k_calib();
        GPIO_DAT_CLR(GPIO_RUN);

        debug("RC32K Cal%d:0x%X\r\n", i, rc32cal);
    }

    // Configure RTC module
    rtc_conf(true);
    rtc_alarm_set(RTC_ALARM_TIME);
    NVIC_EnableIRQ(RTC_IRQn);

    // Configure ATMR: 1us tick, periodic mode with update interrupt
    atmr_init(TMR_PSC, TMR_ARR);
    atmr_ctrl(TMR_PERIOD_MODE, TMR_IR_UI_BIT);
    NVIC_EnableIRQ(ATMR_IRQn);

    __enable_irq();

    // Print initial time info
    time = rtc_time_get();
    alarm = rtc_alarm_get();
    debug("0-RTC(Time:%" PRIu32 ".%03" PRIu32 ",Alarm:%" PRIu32 ".%03" PRIu32 ")\r\n",
          time.sec, time.ms, alarm.sec, alarm.ms);

    // Main loop: monitor RTC interrupt events
    while (1)
    {
        GPIO_DAT_SET(GPIO_RUN);

        if (rtcFlg)
        {
            rtcFlg = false;

            // Print ATMR capture for timing accuracy verification
            debug("TMR(Cnt:%03" PRIu16 ",Val:0x%" PRIX32 ")\r\n", tmrCnt, tmrVal);

            // Print current RTC time and next alarm
            time = rtc_time_get();
            alarm = rtc_alarm_get();
            debug("1-RTC(Time:%" PRIu32 ".%03" PRIu32 ",Alarm:%" PRIu32 ".%03" PRIu32 ")\r\n",
                  time.sec, time.ms, alarm.sec, alarm.ms);
        }

        GPIO_DAT_CLR(GPIO_RUN);
    }
}
