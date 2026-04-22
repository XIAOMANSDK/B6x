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
#define SYS_CLK                 (0)

/// Debug Mode: 0=Disable, 1=via UART, 2=RTT Viewer
#define DBG_MODE                (1)
#define DBG_HARDFAULT           (0)
#define DBG_UART_TXD            (8) // PA08
#define DBG_UART_RXD            (9) // PA09
#define DBG_UART_BAUD           (BRR_921600)

#define CFG_TEST                (1)
#define DBG_NTAG                (1)

#endif  //_APP_CFG_H_
