/**
 ****************************************************************************************
 *
 * @file stepper.h
 *
 * @brief Stepper motor driver interface.
 *
 ****************************************************************************************
 */

#ifndef _STEPPER_H_
#define _STEPPER_H_

#include <stdint.h>
#include <stdbool.h>

/**
 ****************************************************************************************
 * @brief Initialize stepper motor GPIO and state.
 ****************************************************************************************
 */
void stepper_init(void);

/**
 ****************************************************************************************
 * @brief Start stepper motion with acceleration.
 *
 * @param[in] steps           Step count (positive=CW, negative=CCW)
 * @param[in] rpm             Target speed in RPM
 * @param[in] accel_sps2      Acceleration in steps/s^2
 ****************************************************************************************
 */
void stepper_move(int32_t steps, uint8_t rpm, uint32_t accel_sps2);

/**
 ****************************************************************************************
 * @brief Stop stepper motor immediately.
 ****************************************************************************************
 */
void stepper_stop(void);

/**
 ****************************************************************************************
 * @brief Check if stepper motor is currently moving.
 *
 * @return true if busy, false if idle
 ****************************************************************************************
 */
bool stepper_is_busy(void);

#endif
