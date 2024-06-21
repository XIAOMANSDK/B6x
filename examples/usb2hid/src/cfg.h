/**
 ****************************************************************************************
 *
 * @file cfg.h
 *
 * @brief App Configure MACRO, --preinclude
 *
 ****************************************************************************************
 */

#ifndef _APP_CFG_H_
#define _APP_CFG_H_

#define DEMO_HID_BOOT       (0)
#define DEMO_HID_CUSTOM     (1)

#if (DEMO_HID_BOOT + DEMO_HID_CUSTOM != 1)
#error "Select only 1 demo to test!"
#endif

/// System Clock(0=16MHz, 1=32MHz, 2=48MHz, 3=64MHz)
#define SYS_CLK             (2)

/// Debug Mode: 0=Disable, 1=via UART, 2=RTT Viewer
#define DBG_MODE            (1)

#define DBG_UART_TXD        (2) // PA02
#define DBG_UART_RXD        (3) // PA03

/// USB Debug Level: 0=Disable, 1=Error, 2=Warning
#if (DBG_MODE)
#define USB_DBG_LEVEL       (1)
#endif

#if (DEMO_HID_BOOT)
#define USE_KEYS            (0)
#endif

#endif  //_APP_CFG_H_
