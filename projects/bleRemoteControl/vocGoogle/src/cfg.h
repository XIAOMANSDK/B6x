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
#define SYS_CLK                 (2)
#define BLE_LITELIB             (1)

/// Debug Mode(0=Disable, 1=via UART, 2=via RTT)
#define DBG_MODE                (1)
#define DBG_UART_BAUD           (BRR_921600)

#define DBG_UART_TXD            (6) //PA06
#define DBG_UART_RXD            (16) //PA07 用于测量时间，把rx指定到没用到的16上

/// BLE Configure (Single or Multi-Connections)
#define BLE_NB_SLAVE            (1)
#define BLE_NB_MASTER           (0)

#define BLE_ADDR                {0xEB, 0x74, 0x00, 0x00, 0x00, 0xC0}
#if (1)
#define VOICE                  (1)
    // Xiaomi RC
#define BLE_DEV_NAME           "Onn-Remote"
#define BLE_DEV_ICON           0x03C1  // 03C0-Generic HID,03C1-Keyboard,03C2-Mouse,03C4-Gamepad	
#define ADPCM_BLOCK_SIZE       (120)

// vertion
#define CHIP_MODEL      "B66"
#define SDK_VER         "1P4P1"
#define HW_VERSION      "1P0"

#define PROJ_NAME       "RCU"
#define PROJ_VERSION    "V001"
#define PROJ_BUILD_DATE __DATE__
#define PROJ_BUILD_TIME __TIME__
#endif


#define BLE_PHY                 (GAP_PHY_LE_1MBPS)
#define BLE_AUTH                (GAP_AUTH_REQ_NO_MITM_BOND)
#define BLE_EN_SMP              (1)
#define BLE_MTU                 (247)

#define SLV_INTV_MIN            (16)
#define SLV_LATENCY             (49)
#define SLV_TIME_OUT            (1600)
#define GAP_ATT_CFG             (0x40) //(GAP_ATT_SLV_PREF_PAR_BIT)

/// Profile Configure
#define PRF_DISS                (1)
#define PRF_BASS                (1)
#define PRF_HIDS                (1)
#define PRF_SESS                (1)

#define DBG_HARDFAULT           (1)

/// Debug Configure
#if (DBG_MODE) 
    #define DBG_APP             (0)
    #define DBG_PROC            (1)
    #define DBG_ACTV            (0)
    #define DBG_GAPM            (0)
    #define DBG_GAPC            (1)
    #define DBG_KEYS            (1)
    #define DBG_HIDS            (0)
    #define DBG_BASS            (0)
    #define DBG_PT_LOG          (1)
#endif

/// Misc Options
#define CFG_SFT_TMR            (0)
#define CFG_SLEEP              (1)
#define CFG_POWEROFF           (1)
#define RC32K_CALIB_PERIOD     (10000)
#define NOKEY_PRESS_UPDATE_PARAM (3000) // 10s(unit in 1ms)
#define KEY_SCAN_PERIOD        (50)

#define G_NO_ACTION_CNT        (10)  // 无操作n*RC32K_CALIB_PERIOD, 进入睡眠
#endif  //_APP_CFG_H_
