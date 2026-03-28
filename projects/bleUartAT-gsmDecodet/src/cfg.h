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
#define SYS_CLK                (3)  // 64MHz for GSM decoder optimization
#define AT_UART_PORT           (0)

// GSM Decoder Configuration
#define GSM_DECODE_EN          (1)     // Enable GSM decoder
#define GSM_FLASH_ADDR         (0x18020000)  // Flash address for GSM data


// PWM Audio Output
#define GSM_PWM_FREQ           (64000) // PWM frequency for audio output
#define GSM_SAMPLE_RATE        (8000)  // GSM sample rate

// BLE MTU Configuration
#define BLE_MTU_SIZE           (247)   // Maximum MTU size

/// Debug Mode(0=Disable, 1=via UART, 2=via RTT)
#define DBG_MODE               (1)

/// BLE Configure
#define BLE_NB_SLAVE           (1)
#define BLE_NB_MASTER          (0)
#define BLE_EN_SMP             (0)

#define BLE_ADDR               {0x55, 0x08, 0x33, 0xA1, 0x01, 0xD2}
#define BLE_DEV_NAME           "gsmBle-AT"
#define BLE_DEV_ICON           0x0000  // 03C0-Generic HID,03C1-Keyboard,03C2-Mouse,03C4-Gamepad

/// Use ble6_lite.lib
#define BLE_LITELIB            (1)

/// Profile Configure
#define PRF_DISS               (0)
#define PRF_SESS               (1)
#define PRF_SESC               (0)
#define PRF_PTSS               (0)
#define PRF_OTAS               (0)
#define GATT_CLI               (0)
#define SES_UUID_128           (0)

/// Debug Configure
#if (DBG_MODE)
    #define DBG_APP            (1)
    #define DBG_PROC           (1)
    #define DBG_ACTV           (1)
    #define DBG_GAPM           (1)
    #define DBG_GAPC           (1)
    #define DBG_GATT           (0)
    #define DBG_DISS           (0)
    #define DBG_SESS           (0)
    #define DBG_ATCMD          (1)
#endif

/// Misc Options
#define LED_PLAY               (0)
#define CFG_SFT_TMR            (0)

#define CFG_UART_DMA            1
#endif  //_APP_CFG_H_
