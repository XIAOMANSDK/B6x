/**
 ****************************************************************************************
 *
 * @file pwm.h
 *
 * @brief Header file - PWM Driver
 *
 ****************************************************************************************
 */

#ifndef _PWM_H_
#define _PWM_H_

#include <stdint.h>


/*
 * DEFINES
 ****************************************************************************************
 */

enum pwm_timer
{
    // timer for Common Timer
    PWM_CTMR           = 0,
    // timer for Advance Timer
    PWM_ATMR           = 1,
};

enum pwm_channel
{
    // Channels of Common Timer
    PWM_CTMR_CH1       = 0,
    PWM_CTMR_CH2       = 1,
    PWM_CTMR_CH3       = 2,
    PWM_CTMR_CH4       = 3,
    
    // Channels of Advance Timer
    PWM_ATMR_CH1P      = 4,
    PWM_ATMR_CH2P      = 5,
    PWM_ATMR_CH3P      = 6,
    PWM_ATMR_CH4P      = 7,
    PWM_ATMR_CH1N      = 8,
    PWM_ATMR_CH2N      = 9,
    PWM_ATMR_CH3N      = 10,
};

enum pwc_psc
{
    PWC_PSC0           = 0,
    PWC_PSC2           = 1,
    PWC_PSC4           = 2,
    PWC_PSC8           = 3,
};

/// Bits field of Capture/Compare enable register
enum pwm_ccer_bfs
{
    // PWM Enable Bit
    PWM_CCxE_POS       = 0,
    PWM_CCxE_BIT       = (1 << PWM_CCxE_POS),
    // PWM Polar Bit (0-high, 1-low)
    PWM_CCxP_POS       = 1,
    PWM_CCxP_BIT       = (1 << PWM_CCxP_POS),
    // PWM Complementary Enable Bit
    PWM_CCxNE_POS      = 2,
    PWM_CCxNE_BIT      = (1 << PWM_CCxNE_POS),
    // PWM Complementary Polar Bit
    PWM_CCxNP_POS      = 3,
    PWM_CCxNP_BIT      = (1 << PWM_CCxNP_POS),
    // PWM DMA request enable
    PWM_CCxDE_POS      = 4,
    PWM_CCxDE_BIT      = (1 << PWM_CCxDE_POS),
};

#define PWM_CCER_SIPH       (PWM_CCxE_BIT)
#define PWM_CCER_SIPL       (PWM_CCxE_BIT | PWM_CCxP_BIT)

#define PWC_CCER_POSEDGE    (PWM_CCxE_BIT) // rising-edge
#define PWC_CCER_NEGEDGE    (PWM_CCxE_BIT | PWM_CCxP_BIT) // falling-edge

/// Bits field of Capture/Compare mode register
enum pwm_ccmr_bfs
{
    // Capture/Compare selection - bit[1:0]
    PWM_CCxS_LSB       = 0,
    PWM_CCxS_MSK       = (0x03 << PWM_CCxS_LSB),
    
    // Output Compare fast enable - bit2
    PWM_OCxFE_POS      = 2,
    PWM_OCxFE_BIT      = (1 << PWM_OCxFE_POS),
    // Output Compare preload enable - bit3
    PWM_OCxPE_POS      = 3,
    PWM_OCxPE_BIT      = (1 << PWM_OCxPE_POS),
    // Output Compare mode - bit[6:4]
    PWM_OCxM_LSB       = 4,
    PWM_OCxM_MSK       = (0x07 << PWM_OCxM_LSB),
    // Output Compare clear enable - bit7
    PWM_OCxCE_POS      = 7,
    PWM_OCxCE_BIT      = (1 << PWM_OCxCE_POS),
    
    // Input capture prescaler - bit[3:2]
    PWM_ICxPSC_LSB     = 2,
    PWM_ICxPSC_MSK     = (0x03 << PWM_ICxPSC_LSB),
    // Input capture filter - bit[7:4]
    PWM_ICxF_LSB       = 4,
    PWM_ICxF_MSK       = (0x0F << PWM_ICxF_LSB),
};

#define PWM_CCxS(sel)       (((sel) & 0x03) << PWM_CCxS_LSB)
#define PWM_OCxM(mod)       (((mod) & 0x07) << PWM_OCxM_LSB)
#define PWM_ICxPSC(psc)     (((psc) & 0x03) << PWM_ICxPSC_LSB)
#define PWM_ICxF(flt)       (((flt) & 0x0F) << PWM_ICxF_LSB)

#define PWM_CCMR_MODE1      (PWM_OCxPE_BIT | PWM_OCxM(6))
#define PWM_CCMR_MODE2      (PWM_OCxPE_BIT | PWM_OCxM(7))

#define PWC_CCMR_MODE(sel, flt, psc)  \
                            (PWM_CCxS(sel) | PWM_ICxF(flt) | PWM_ICxPSC(psc))

typedef struct pwm_channel_cfg
{
    // Capture/Compare mode
    uint8_t  ccmr;
    // Capture/Compare enable
    uint8_t  ccer;
    // Duty ratio = duty / arr
    uint16_t duty;
} pwm_chnl_cfg_t;


/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Init Timer(Source) for PWM Capture/Compare.
 *
 * @param[in] tmr  Timer ID @see enum pwm_timer
 * @param[in] psc  PWM configure prescaler
 * @param[in] arr  PWM configure Auto-reload
 * 
 ****************************************************************************************
 */
void pwm_init(uint8_t tmr, uint16_t psc, uint16_t arr);

/**
 ****************************************************************************************
 * @brief Deinit Timer Module.
 *
 * @param[in] tmr  Timer ID @see enum pwm_timer
 ****************************************************************************************
 */
void pwm_deinit(uint8_t tmr);

/**
 ****************************************************************************************
 * @brief Config Timer(Source) for PWC(PWM Capture).
 *
 * @param[in] tmr   Timer ID @see enum pwm_timer
 * @param[in] smcr  PWM Slave mode control
 * @param[in] intr  PWM Interrupt configure
 * 
 ****************************************************************************************
 */
void pwm_conf(uint8_t tmr, uint16_t smcr, uint16_t intr);

/**
 ****************************************************************************************
 * @brief Start/Enable Timer(Source) for PWM Capture/Compare.
 *
 * @param[in] tmr   Timer ID @see enum pwm_timer
 * 
 ****************************************************************************************
 */
void pwm_start(uint8_t tmr);

/**
 ****************************************************************************************
 * @brief Stop Timer(Source) for PWM Capture/Compare.
 *
 * @param[in] tmr   Timer ID @see enum pwm_timer
 *
 * @Note This API will stop all PWM channels. 
 *       If you want to stop a single PWM channel, please use pwm_chnl_stop API.
 ****************************************************************************************
 */
void pwm_stop(uint8_t tmr);

/**
 ****************************************************************************************
 * @brief Config special channel for PWM Capture/Compare.
 *
 * @param[in] chnl  Channel ID @see enum pwm_channel
 * @param[in] conf  Pointer of channel configure @see struct pwm_channel_cfg
 *                  Channel stop when conf is NULL, else start.
 *
 ****************************************************************************************
 */
void pwm_chnl_set(uint8_t chnl, const pwm_chnl_cfg_t *conf);

/// Macro for Channel clear
#define pwm_chnl_clr(chnl)    pwm_chnl_set(chnl, NULL)

/**
 ****************************************************************************************
 * @brief Update special channel's duty for PWM Capture/Compare.
 *
 * @param[in] chnl  Channel ID @see enum pwm_channel
 * @param[in] duty  Value of Duty, ratio = duty / arr
 *
 ****************************************************************************************
 */
void pwm_duty_upd(uint8_t chnl, uint16_t duty);


#endif // _PWM_H_
