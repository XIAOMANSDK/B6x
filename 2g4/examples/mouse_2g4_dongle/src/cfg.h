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
#define SYS_CLK       (2)

/// Debug Mode: 0=Disable, 1=via UART, 2=RTT Viewer
#define DBG_MODE      (0)
#if (SYS_CLK == 2)
    #define DBG_UART_BAUD       BRR_DIV(921600, 48M)
#else
    #define DBG_UART_BAUD (BRR_921600)
#endif

#define DBG_UART_TXD  (17) // PA17
#define DBG_UART_RXD  (18) // PA18

#if (DBG_MODE)
    #define DBG_USB (1)
    #define DBG_MST (1)
#endif

#define CFG_USB       (1)
// XY_1B = 1, XY_2B = 2, XY_4B = 3
#define XY_nB         (1)

#define CFG_USE_RPT_ID (1)
#if (CFG_USE_RPT_ID)
#define MOUSE_LEN 5
#else
#define MOUSE_LEN 4
#endif

#define MOUSE_IN_EP  (0x81)
#define HID_INST_CNT (1)

#endif  //_APP_CFG_H_
