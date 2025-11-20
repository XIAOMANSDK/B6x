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
#define SYS_CLK                (0)

/// Debug Mode(0=Disable, 1=via UART, 2=via RTT)
#define DBG_MODE               (1)

/// Debug Configure
#if (DBG_MODE)
    #define DBG_PROC           (1)
#endif

/// Misc Options
#define LED_PLAY               (1)
#define CONFIG_STORE_OFFESET   (0x1200)
#define PA_OFFSET      (0)
#define CAP_OFFSET     (1)
#define ADJ00_OFFSET   (2)
#define ADJ12_OFFSET   (3)
#define ADJ05_OFFSET   (4)
#define VCO_ADJ_OFFSET (5)
#define BG_RES_TRIM_OFFSET (6)
#define CONFIG_OFFSET_MAX BG_RES_TRIM_OFFSET

#endif  //_APP_CFG_H_
