/**
 ****************************************************************************************
 *
 * @file keys.h
 *
 * @brief Header file - Keys Scanning and Report
 *
 ****************************************************************************************
 */

#ifndef _KEYS_H_
#define _KEYS_H_

#include <stdint.h>
#include <stdbool.h>

/**
 ****************************************************************************************
 *
 * @brief Initialize GPIO for LEDs (output) and buttons (input with pull-up).
 *
 ****************************************************************************************
 */
void keys_init(void);

/**
 ****************************************************************************************
 *
 * @brief Poll button states and trigger BLE actions.
 *
 * @note Called from user_procedure() in main loop.
 *
 ****************************************************************************************
 */
void keys_scan(void);

#endif  //_KEYS_H_
