/**
 ****************************************************************************************
 *
 * @file proc.h
 *
 * @brief User procedure declarations for BLE-UART-USB bridge.
 *
 ****************************************************************************************
 */

#ifndef _PROC_H_
#define _PROC_H_

/**
 ****************************************************************************************
 * @brief User procedure called in main loop.
 *        Handles UART data accumulation and routes to USB CDC or BLE.
 ****************************************************************************************
 */
void user_procedure(void);

#endif /* _PROC_H_ */
