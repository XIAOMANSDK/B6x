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
#define DBG_MODE               (0)

#define PA_UART1_TX            (6) //PA06
#define PA_UART1_RX            (7) //PA07
#define WAKEUP_IO_MASK         (GPIO07)

/// use lite lib.
#define BLE_LITELIB            (1)

#define BLE_ADDR               {0x1E, 0x30, 0x05, 0x24, 0x01, 0xD2}
#define BLE_DEV_NAME           "HID-UART-"
#define BLE_DEV_ICON           0x03C1  // 03C0-Generic HID,03C1-Keyboard,03C2-Mouse,03C4-Gamepad
#define BLE_PHY                (GAP_PHY_LE_1MBPS)
#define BLE_AUTH               (GAP_AUTH_REQ_NO_MITM_BOND)

/// Profile Configure
#define PRF_DISS               (1)
#define PRF_BASS               (1)
#define PRF_HIDS               (1)

/// Debug Configure
#if (DBG_MODE)
    #define DBG_APP            (1)
    #define DBG_ACTV           (0)
    #define DBG_GAPM           (0)
    #define DBG_GAPC           (1)
    #define DBG_PROC           (0)
    #define DBG_HIDS           (1)
#endif

#define STORE_MAC_OFFSET       (0x1200)

#endif  //_APP_CFG_H_
