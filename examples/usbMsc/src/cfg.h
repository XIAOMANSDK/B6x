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
#define DBG_MODE            (0)

#define DBG_UART_TXD        (8) // PA08
#define DBG_UART_RXD        (9) // PA09
#define DBG_UART_BAUD       (BRR_921600)

/// USB Debug Level: 0=Disable, 1=Error, 2=Warning
#if (DBG_MODE)
#define USB_DBG_LEVEL       (1)
#endif

/// MSC Debug: 0=Disable, 1=Enable (verbose SCSI/CBW/CSW logging)
#define DBG_MSC             (0)

/// Flash Type: 0=Internal Flash, 1=SPI Flash
#define MSC_FLASH_TYPE      (0)

#endif  //_APP_CFG_H_
