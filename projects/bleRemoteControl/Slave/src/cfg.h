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
#define SYS_CLK                 (0)
#define BLE_LITELIB             (1)

/// Debug Mode(0=Disable, 1=via UART, 2=via RTT)
#define DBG_MODE                (0)
#define DBG_UART_BAUD           (BRR_921600)
/// BLE Configure (Single or Multi-Connections)
#define BLE_NB_SLAVE            (1)
#define BLE_NB_MASTER           (0)

#define BLE_ADDR                {0xE0, 0x74, 0x69, 0x56, 0xAB, 0x14}

#if (0)
#define VOICE                  (1)
#define BLE_DEV_NAME           {0xE8,0x93,0x9D,0xE7,0x89,0x99,0xE8,0xAF,0xAD,0xE9,0x9F,0xB3,0xE9,0x81,0xA5,0xE6,0x8E,0xA7,0xE5,0x99,0xA8,0x00} // 蓝牙语音遥控器
#else
#define DBG_UART_TXD            (0) //PA00
#define DBG_UART_RXD            (2) //PA02
#define BLE_DEV_NAME            "BluetoothRemote"
#define INFO_PNP_ID             ("\x02\x49\x77\x6A\x32\x00\x03")  // PID:0x326A VID:0x7749
#endif

#define BLE_DEV_ICON             0x03C1  // 03C0-Generic HID,03C1-Keyboard,03C2-Mouse,03C4-Gamepad
#define BLE_PHY                 (GAP_PHY_LE_1MBPS)
#define BLE_AUTH                (GAP_AUTH_REQ_NO_MITM_BOND)
#define BLE_EN_SMP              (1)
#define BLE_MTU                 (247)

#define APP_ADV_DURATION        (3000) // 30s
#define SLV_INTV_MIN            (16)
#define SLV_LATENCY             (49)
#define SLV_TIME_OUT            (1600)
#define GAP_ATT_CFG             (0x40) //(GAP_ATT_SLV_PREF_PAR_BIT)

/// Profile Configure
#define PRF_DISS                (1)
#define PRF_BASS                (1)
#define PRF_HIDS                (1)
   
/// Debug Configure
#if (DBG_MODE) 
    #define DBG_APP             (1)
    #define DBG_PROC            (0)
    #define DBG_ACTV            (1)
    #define DBG_GAPM            (0)
    #define DBG_GAPC            (1)
    #define DBG_KEYS            (0)
    #define DBG_HIDS            (0)
    #define DBG_BASS            (0)
#endif

/// Misc Options
#define CFG_SFT_TMR            (0)
#define CFG_SLEEP              (1)
#define CFG_POWEROFF           (1)
#define RC32K_CALIB_PERIOD     (10000)
#define NOKEY_PRESS_UPDATE_PARAM (3000) // 10s(unit in 1ms)
#define KEY_SCAN_PERIOD        (50)
#define G_NO_ACTION_CNT        (1)  // 无操作n*RC32K_CALIB_PERIOD, 进入睡眠

#define ADV_LED_GPIO           (GPIO00)
#define IR_TX_PAD              (7)       // PA9  ATMR IO PA7~PA13
#define KEY_IR_PERIOD          (110)     // 红外重复码处理110ms
#define IR_TX_ADR              0x0816    // 地址0x16, 地址反码0x08

#define ADV_LED_PERIOD         (1000)     // LED处理1000ms
#endif  //_APP_CFG_H_
