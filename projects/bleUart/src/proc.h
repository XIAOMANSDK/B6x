/**
 ****************************************************************************************
 *
 * @file proc.h
 *
 * @brief User procedure declarations for BLE-UART transparent transmission.
 *
 ****************************************************************************************
 */

#ifndef _PROC_H_
#define _PROC_H_

/**
 ****************************************************************************************
 * @brief User procedure called in main loop.
 *        Handles sleep, UART-to-BLE data forwarding, and speed test.
 ****************************************************************************************
 */
void user_procedure(void);

#endif /* _PROC_H_ */
