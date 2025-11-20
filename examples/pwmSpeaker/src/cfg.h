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

#define SYS_CLK             (3)
/// Debug Mode: 0=Disable, 1=via UART, 2=RTT Viewer
#define DBG_MODE            (1)

#define DBG_UART_BAUD       (BRR_921600)

#define PA_ATMR_P           (8)           //PWMÊä³öÒý½Å
#endif  //_APP_CFG_H_
