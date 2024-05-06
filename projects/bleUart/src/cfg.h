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

#define BLE_ADDR               {0x26, 0x08, 0x33, 0xA1, 0x01, 0xD2}
#define BLE_DEV_NAME           "myBle-Uart"
#define BLE_DEV_ICON           0x0000

#define BLE_MTU                (185)
#define SES_RXD_MAX_LEN        (0x200)

#define APP_ADV_INT_MIN        (32) // unit in 0.625ms
#define APP_ADV_INT_MAX        (32) // unit in 0.625ms

/// Profile Configure
#define PRF_DISS               (1)
#define PRF_SESS               (1)

/// Serial Service @see prf_sess.h
#define SES_UUID_128           (1)
#define SES_READ_SUP           (0)

/// Debug Configure
#if (DBG_MODE)
    #define DBG_APP            (0)
    #define DBG_PROC           (0)
    #define DBG_ACTV           (0)
    #define DBG_GAPM           (0)
    #define DBG_GAPC           (1)
    #define DBG_DISS           (0)
    #define DBG_SESS           (0)
#endif

/// Misc Options
#define LED_PLAY               (0)
#define CFG_SLEEP              (0)

#endif  //_APP_CFG_H_
