/**
 ****************************************************************************************
 *
 * @file rco.h
 *
 * @brief Header file - RCO(32k, 16M) Driver
 *
 ****************************************************************************************
 */

#ifndef _RCO_H_
#define _RCO_H_

#include <stdint.h>


/*
 * DEFINES
 ****************************************************************************************
 */

/// Reference Clock for rc32k calibration
enum rc32k_ref_clk
{
    RCLK_HSE                 = 0, // XOSC16M
    RCLK_DPLL                = 1,
    RCLK_DPLL128             = 2,
};

/// Config Control for rc32k calibration
enum rc32k_cal_ctrl
{
    RCAL_CYCLES_LSB          = 0,
    RCAL_CYCLES_MSK          = 0x1F,
    RCAL_SCALE_EN            = (1 << 5),
    RCAL_DELAY_EN            = (1 << 6),
};

#define RCAL_CYCLES(n)         ((n) << RCAL_CYCLES_LSB)

/// Trimming value for rc32k calibration
typedef union
{
    struct
    {
        uint16_t msb_val:     4;
        uint16_t lsb_val:    10;
        uint16_t reserve:     2;
    };
    uint16_t value;
} rc32k_trim_val;


/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Enable and Config rc32k
 *
 * @param[in] ref_clk   reference clock for calibration @see enum rc32k_ref_clk
 * @param[in] cal_ctrl  config control for calibration @see enum rc32k_cal_ctrl
 * 
 ****************************************************************************************
 */
void rc32k_conf(uint8_t ref_clk, uint8_t cal_ctrl);

/**
 ****************************************************************************************
 * @brief Start rc32k calibration, after rc32k_conf().
 *
 * @return rc32k trim value
 ****************************************************************************************
 */
uint16_t rc32k_calib(void);

/**
 ****************************************************************************************
 * @brief Direct Set trim value to calibrate rc32k.
 *
 * @param[in] value   calib-target value
 *
 ****************************************************************************************
 */
void rc32k_trim_set(uint16_t value);

/**
 ****************************************************************************************
 * @brief Get current value of rc32k calibrated or used.
 *
 * @return calib-target value
 *
 ****************************************************************************************
 */
uint16_t rc32k_trim_get(void);

/**
 ****************************************************************************************
 * @brief Direct Set trim value to calibrate rc16m.
 *
 * @param[in] value   calib-target value 0~63
 *
 ****************************************************************************************
 */
void rc16m_trim_set(uint8_t value);

/**
 ****************************************************************************************
 * @brief Get current value of rc16m calibrated or used.
 *
 * @return calib-target value 0~63
 *
 ****************************************************************************************
 */
uint8_t rc16m_trim_get(void);

#if (ROM_UNUSED)
/**
 ****************************************************************************************
 * @brief Start rc16m calibration in dichotomy.
 *
 * @return rc16m trim value (0~63)
 ****************************************************************************************
 */
uint8_t rc16m_calib(void);
#endif

#endif // _RCO_H_
