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

/// BLE Configure (Single or Multi-Connections)
#define BLE_NB_SLAVE           (1)
#define BLE_NB_MASTER          (0)

#define BLE_ADDR               {0x29, 0x08, 0x33, 0xA1, 0x01, 0xD2}
#define BLE_DEV_NAME           "B6x-HID"
#define BLE_DEV_ICON           0x03C1  // 03C0-Generic HID,03C1-Keyboard,03C2-Mouse,03C4-Gamepad
#define BLE_PHY                (GAP_PHY_LE_1MBPS)
#define BLE_AUTH               (GAP_AUTH_REQ_NO_MITM_BOND)
#define BLE_DBG_LTK            (1)

/// Profile Configure
#define PRF_DISS               (1)
#define PRF_BASS               (1)
#define PRF_HIDS               (1)

/// Debug Configure
#if (DBG_MODE)
    #define DBG_APP            (1)
    #define DBG_PROC           (1)
    #define DBG_ACTV           (0)
    #define DBG_GAPM           (1)
    #define DBG_GAPC           (1)
    #define DBG_HIDS           (1)
    #define DBG_BASS           (0)
#endif

/// Misc Options
#define LED_PLAY               (1)

#endif  //_APP_CFG_H_
