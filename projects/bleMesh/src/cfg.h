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
#define UART1_CONF_BAUD        (BRR_921600)
#define DBG_HARDFAULT          (0)

/// Redefine BLE Heap Size, Note: Mesh's STACK Size ~2.5K
#define BLE_HEAP_BASE          (0x20004800)
#define BLE_HEAP_ENV_SIZE      (0xD00)
#define BLE_HEAP_MSG_SIZE      (0x2000)

#define BLE_PHY                (GAP_PHY_LE_1MBPS)
#define BLE_ROLE               (GAP_ROLE_PERIPHERAL | GAP_ROLE_OBSERVER)

/// BLE Configure (Single or Multi-Connections)
#define BLE_NB_SLAVE           (0)
#define BLE_NB_MASTER          (0)

#define BLE_ADDR               {0x2e, 0x1e, 0x49, 0x65, 0x42, 0xFC} // *MUST Changed for multi devices
#define BLE_DEV_NAME           "myBle-Mesh"
#define BLE_DEV_ICON           0x0000

#define APP_ADV_INT_MIN        (32) // unit in 0.625ms
#define APP_ADV_INT_MAX        (32) // unit in 0.625ms

/// Profile Configure
#define PRF_GATT               (0)
#define PRF_DISS               (0)
#define PRF_SESS               (0)
#define PRF_MESH               (1)

/// Serial Service @see prf_sess.h
#define SES_UUID_128           (1)
#define SES_READ_SUP           (0)

/// Debug Configure
#if (DBG_MODE)
    #define DBG_APP            (1)
    #define DBG_PROC           (0)
    #define DBG_ACTV           (0)
    #define DBG_GAPM           (0)
    #define DBG_GAPC           (1)
    #define DBG_MESH           (1)
    
    #define DBG_DISS           (0)
    #define DBG_SESS           (0)
    
#endif

/// Misc Options
#define LED_PLAY               (0)
#define CFG_SLEEP              (0)


#endif  //_APP_CFG_H_
