/**
 ****************************************************************************************
 *
 * @file rco_test.c
 *
 * @brief RC32K/RC16M calibration and drift monitoring test
 *
 * @details
 * Demonstrates RC oscillator calibration and monitoring:
 * - Calibrate RC32K using DPLL reference clock
 * - Calibrate RC16M via binary search
 * - Monitor oscillator frequency stability in real-time
 * - Log calibration and drift events with RTC timestamps
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "drvs.h"
#include "dbg.h"
#include "utils.h"


/*
 * DEFINES
 ****************************************************************************************
 */

/// GPIO pins for status indication and clock output
#define GPIO_32K            GPIO11  ///< RC32K calibration status indicator
#define GPIO_16M            GPIO12  ///< RC16M calibration status indicator
#define GPIO_RUN            GPIO13  ///< Main loop running status indicator

/// RC32K calibration parameters
#define RC32K_REF_CLK       RCLK_DPLL      ///< Reference clock: DPLL (high precision)
#define RC32K_CAL_CTL       RCAL_CYCLES(4) ///< Calibration control: 4 cycles
#define RC32K_CAL_NB        8              ///< Number of calibration iterations
#define RC32K_CAL_DIFF      2              ///< LSB drift threshold

/*
 * FUNCTIONS
 ****************************************************************************************
 */

#if (RC32K_TEST)

static uint16_t rc32kCal;  ///< RC32K calibration baseline for drift comparison

/**
 ****************************************************************************************
 * @brief Initialize and calibrate RC32K oscillator
 *
 * @details
 * Configure RC32K calibration parameters and run multiple calibration passes
 * to obtain a stable trim value. Output each pass result for observation.
 ****************************************************************************************
 */
static void rc32kInit(void)
{
    iospc_clkout(CLK_OUT_LSI);

    rc32k_conf(RC32K_REF_CLK, RC32K_CAL_CTL);

    for (uint8_t i = 0; i < RC32K_CAL_NB; i++)
    {
        GPIO_DAT_SET(GPIO_32K);
        rc32kCal = rc32k_calib();
        GPIO_DAT_CLR(GPIO_32K);

        debug("RC32K Cal%d(msb:0x%X,lsb:0x%X)\r\n", i, rc32kCal & 0xF, rc32kCal >> 4);
    }
}

/**
 ****************************************************************************************
 * @brief Check if RC32K frequency has drifted
 *
 * @return true  Frequency drift detected (MSB changed or LSB exceeded threshold)
 * @return false Frequency is stable
 *
 * @details
 * Run a single calibration and compare with the stored baseline.
 * Update the baseline when drift is detected.
 ****************************************************************************************
 */
static bool rc32kChgd(void)
{
    uint16_t curcal;

    GPIO_DAT_SET(GPIO_32K);
    curcal = rc32k_calib();
    GPIO_DAT_CLR(GPIO_32K);

    if (((curcal & 0xF) != (rc32kCal & 0xF))
        || (co_abs((int)(curcal >> 4) - (int)(rc32kCal >> 4)) > RC32K_CAL_DIFF))
    {
        debug("RC32K Changed(msb:0x%X->0x%X,lsb:0x%X->0x%X)\r\n",
              rc32kCal & 0xF, curcal & 0xF, rc32kCal >> 4, curcal >> 4);
        rc32kCal = curcal;
        return true;
    }
    return false;
}
#endif

#if (RC16M_TEST)
static uint8_t rc16mCal;  ///< RC16M calibration baseline for drift comparison

/**
 ****************************************************************************************
 * @brief Initialize and calibrate RC16M oscillator
 *
 * @details
 * Configure clock output and run binary-search calibration to find the
 * optimal trim value for 16 MHz.
 ****************************************************************************************
 */
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

/**
 ****************************************************************************************
 * @brief Check if RC16M frequency has drifted
 *
 * @return true  Trim value changed (drift detected)
 * @return false Frequency is stable
 ****************************************************************************************
 */
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

/**
 ****************************************************************************************
 * @brief RCO test main function
 *
 * @details
 * Run RC oscillator calibration and continuous drift monitoring:
 * 1. GPIO setup for status indicators
 * 2. RC16M initial calibration (if enabled)
 * 3. RC32K initial calibration (if enabled)
 * 4. RTC setup for drift event timestamps
 * 5. Main loop: poll drift and log RTC time on change
 ****************************************************************************************
 */
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
    debug("RTC Time:%d.%03d\r\n", (int)time.sec, (int)time.ms);

    while (1)
    {
        GPIO_DAT_SET(GPIO_RUN);

        #if (RC16M_TEST)
        if (rc16mChgd()) { chgd = true; }
        #endif

        #if (RC32K_TEST)
        if (rc32kChgd()) { chgd = true; }
        #endif

        if (chgd)
        {
            chgd = false;
            time = rtc_time_get();
            debug("at RTC Time:%d.%03d\r\n", (int)time.sec, (int)time.ms);
        }

        GPIO_DAT_CLR(GPIO_RUN);
    }
}
