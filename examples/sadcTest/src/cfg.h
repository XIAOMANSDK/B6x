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

#define DMA_USED            (1)

#if (DMA_USED)
#define SAMP_ADTMR          (0)
#define SAMP_PCM            (1)
#endif

#endif  //_APP_CFG_H_
