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

/// Debug Mode: 0=Disable, 1=via UART, 2=RTT Viewer
#define DBG_MODE            (1)
#define DBG_IR              (1)

#define DBG_UART_TXD        (6) // PA6
#define DBG_UART_RXD        (7) // PA7

#define IR_TX_PAD           (9) // PA7  ATMR IO PA7~PA13
#define IR_TX_ADR           0x00FF
#endif  //_APP_CFG_H_
