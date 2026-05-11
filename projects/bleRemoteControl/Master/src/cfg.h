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

#define CFG_USB                (1)

#define CFG_USE_RPT_ID         (0)

/// System Clock(0=16MHz, 1=32MHz, 2=48MHz, 3=64MHz)
#define SYS_CLK                (2)

/// Debug Mode(0=Disable, 1=via UART, 2=via RTT)
#define DBG_MODE               (0)
#if (CFG_USB)
#define DBG_UART_TXD           (13)
#define DBG_UART_RXD           (18)
#else
#define DBG_UART_TXD           (6)
#define DBG_UART_RXD           (18)
#endif

#define PA_UART1_TX            (DBG_UART_TXD)
#define PA_UART1_RX            (DBG_UART_RXD)
//#if (SYS_CLK)
//#define DBG_UART_BAUD          BRR_DIV(2000000, 48M)
//#else
//#define DBG_UART_BAUD          BRR_DIV(2000000, 48M)
//#endif
//#define UART1_CONF_BAUD        DBG_UART_BAUD

/// BLE Configure (Single or Multi-Connections)
#define BLE_NB_SLAVE           (0)
#define BLE_NB_MASTER          (1)
#define BLE_ADDR               {0x2C, 0x06, 0x01, 0x24, 0x20, 0xD2}
#define BLE_DEV_NAME           "usb_receiver_"
#define BLE_DEV_ICON           0x0000
#define BLE_AUTH               (GAP_AUTH_REQ_NO_MITM_NO_BOND)
#define BLE_EN_SMP             (0)
//#define BLE_SYNC_WORD          (0x26D5EF45)
#define BLE_MTU               (251)
#define SCAN_NUM_MAX           (1)
#define CFG_HW_TIMER           (0)
#define BLE_HEAP_BASE          (0x20004E00)

#define BT_MAC_STORE_OFFSET    (0x1200)

/// Profile Configure
#define GATT_CLI               (1)

/// Debug Configure
#if (DBG_MODE)
    #define DBG_APP            (1)
    #define DBG_PROC           (0)
    #define DBG_ACTV           (1)
    #define DBG_GAPM           (0)
    #define DBG_GAPC           (1)
    #define DBG_GATT           (1)
    #define DBG_USB            (0)
#endif

/// USB&BLE CFG
#define GATT_KB_HDL  (45)
#define GATT_MIC_HDL (56)
#define KB_LEN       (8)
#define MIC_LEN      (64)

#endif  //_APP_CFG_H_
