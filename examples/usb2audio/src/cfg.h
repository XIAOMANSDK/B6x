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

#define DEMO_AUDIO_MIC      (1)
#define DEMO_AUDIO_HID      (0)

#if (DEMO_AUDIO_MIC + DEMO_AUDIO_HID != 1)
#error "Select only 1 demo to test!"
#endif

/// System Clock(0=16MHz, 1=32MHz, 2=48MHz, 3=64MHz)
#define SYS_CLK             (2)

/// Debug Mode: 0=Disable, 1=via UART, 2=RTT Viewer
#define DBG_MODE            (1)

#define DBG_UART_TXD        (8) // PA08
#define DBG_UART_RXD        (9) // PA09
#define DBG_UART_BAUD       (BRR_921600)

#define PCM_SAMPLE_NB       (8) // 256: 32ms  8000Hz

/// USB Debug Level: 0=Disable, 1=Error, 2=Warning
#if (DBG_MODE)
#define USB_DBG_LEVEL       (1)
#endif

#endif  //_APP_CFG_H_
