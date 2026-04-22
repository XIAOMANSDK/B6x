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

/// System Clock(0=16MHz, 1=32MHz, 2=48MHz, 3=64MHz)
#define SYS_CLK             (2)

/// Debug Mode: 0=Disable, 1=via UART, 2=RTT Viewer
#define DBG_MODE            (1)
#define DBG_ISO7816         (1)

#define DBG_UART_BAUD       (BRR_921600)
#define DBG_UART_TXD        (6) // PA06
#define DBG_UART_RXD        (7) // PA07

/// ISO7816 smart card interface pins
#define PA_ISO7816_DATA     (13)
#define PA_ISO7816_CLK      (14)
#define PA_ISO7816_RST      (15)

/// ESAM VCC control pin
#define PA_ESAM_VCC         (16)

#endif  //_APP_CFG_H_
