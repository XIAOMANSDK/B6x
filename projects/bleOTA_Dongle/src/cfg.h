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
#define SYS_CLK (1)

/// Debug Mode(0=Disable, 1=via UART, 2=via RTT)
#define DBG_MODE (1)

#define DBG_UART_TXD (2) // PA02
#define PA_UART1_TX  DBG_UART_TXD
#define PA_UART1_RX  (3)

/// BLE Configure (Single or Multi-Connections)
#define BLE_NB_SLAVE  (0)
#define BLE_NB_MASTER (1)
#define BLE_ADDR      { 0x2C, 0x28, 0x08, 0x23, 0x20, 0xD2 }
#define BLE_DEV_NAME  "BXOTA-Dongle"
#define BLE_DEV_ICON  0x0000

#define BLE_EN_SMP    (0)
#define SCAN_ADV_NAME "BXOTA-"
#define SCAN_NUM_MAX  (1)

#define SFTMR_SRC (0) //(0-TMS_SysTick)
#define BLE_MTU   (512)

/// Profile Configure
#define GATT_CLI (1)

/// Debug Configure
#if (DBG_MODE)
#define DBG_APP  (1)
#define DBG_PROC (1)
#define DBG_ACTV (1)
#define DBG_GAPM (0)
#define DBG_GAPC (1)
#define DBG_GATT (0)
#endif

#define LED_PLAY 1

// PA8/9/10
#define LED_GROUP 1

#endif //_APP_CFG_H_
