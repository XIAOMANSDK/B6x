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

/// System Clock (0=16MHz, 1=32MHz, 2=48MHz, 3=64MHz)
#define SYS_CLK                (0)

/// Debug Mode (0=Disable, 1=via UART, 2=via RTT)
#define DBG_MODE               (1)

/// Debug Configure
#if (DBG_MODE)
    #define DBG_PROC           (1)
#endif

/// Misc Options
#define LED_PLAY               (1)

/// Flash config storage offset (must be 256-byte aligned)
#define CONFIG_STORE_OFFSET    (0x1200)

/// Config field offsets within Fcc_Config_t
#define PA_OFFSET              (0)
#define CAP_OFFSET             (1)
#define ADJ00_OFFSET           (2)
#define ADJ12_OFFSET           (3)
#define ADJ05_OFFSET           (4)
#define VCO_ADJ_OFFSET         (5)
#define BG_RES_TRIM_OFFSET     (6)
#define CONFIG_OFFSET_MAX      BG_RES_TRIM_OFFSET

/// Default config values (used when Flash is erased)
#define CFG_DEFAULT_TX_POWER   (0x0B)
#define CFG_DEFAULT_CAP        (0x14)
#define CFG_DEFAULT_VCO_ADJ    (0x03)
#define CFG_ERASED_BYTE        (0xFF)

/// Range limits for config fields
#define CFG_PA_MAX             (0x0F)
#define CFG_ADJ_MAX            (0x3F)
#define CFG_BG_RES_MAX         (0x1F)

#endif  //_APP_CFG_H_
