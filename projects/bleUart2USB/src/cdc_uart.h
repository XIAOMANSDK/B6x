/**
 ****************************************************************************************
 *
 * @file cdc_uart.h
 *
 * @brief USB CDC and UART bridge initialization and definitions.
 *
 ****************************************************************************************
 */

#ifndef _CDC_UART_H_
#define _CDC_UART_H_

#include <stdint.h>

/** CDC0 bulk IN endpoint address */
#define CDC0_IN_EP             (0x81)

/**
 ****************************************************************************************
 * @brief Initialize USB CDC device with descriptors and endpoints.
 ****************************************************************************************
 */
void usbdInit(void);

#endif /* _CDC_UART_H_ */
