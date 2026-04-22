/**
 ****************************************************************************************
 *
 * @file cdc_uart.h
 *
 * @brief USB CDC virtual serial port interface
 *
 ****************************************************************************************
 */

#ifndef _CDC_UART_H_
#define _CDC_UART_H_

#include <stdint.h>

/**
 ****************************************************************************************
 * @brief Initialize USB device with CDC class
 ****************************************************************************************
 */
void usbdInit(void);

/**
 ****************************************************************************************
 * @brief USB CDC test loop (call in main while-1)
 *
 * @details Sends test data when host opens the virtual COM port (DTR active)
 ****************************************************************************************
 */
void usbdTest(void);

#endif /* _CDC_UART_H_ */
