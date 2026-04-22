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
#define DBG_UART_BAUD       (BRR_921600)

#if (DBG_MODE)
    #define DBG_SLV (1)
#endif

#define MASTER_TEST          (0)
#define CFG_TEST_CIRCLE_DATA (1)

#define MOUSE_LEN 5
#endif  //_APP_CFG_H_
