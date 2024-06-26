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
#define DBG_UART_BAUD          (BRR_921600)
#define UART_CMD               (1)
#define UART1_CONF_BAUD        (DBG_UART_BAUD)

// only large lib support 3Master+3Slave
#define BLE_LARGELIB           (1)

/// BLE Configure (Single or Multi-Connections)
#define BLE_NB_MASTER          (3)
#define BLE_NB_SLAVE           (3)

#define BLE_ADDR               {0x00, 0x08, 0x33, 0xA1, 0x01, 0xD2}
#define BLE_DEV_NAME           "myBle-3M3S-"
#define BLE_DEV_ICON           0x0000  // 03C0-Generic HID,03C1-Keyboard,03C2-Mouse,03C4-Gamepad


/// Profile Configure
#define PRF_DISS               (BLE_NB_SLAVE)
#define PRF_SESS               (BLE_NB_SLAVE)

/// GATT Client and debugLTK Used
#define GATT_CLI               (BLE_NB_MASTER)
#define BLE_DBG_LTK            (BLE_NB_MASTER)

/// Debug Configure
#if (DBG_MODE)
    #define DBG_APP            (1)
    #define DBG_PROC           (1)
    #define DBG_ACTV           (1)
    #define DBG_GAPM           (0)
    #define DBG_GAPC           (1)
    #define DBG_GATT           (1)
    
    #define DBG_SESS           (1)
    #define DBG_KEYS           (0)
#endif

/// Misc Options
#define LED_PLAY               (0)


#endif  //_APP_CFG_H_
