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
#define SFTMR_NUM              (8)

#define PA_UART1_RTS           (4) //PA04
#define PA_UART1_CTS           (5) //PA05
#define PA_UART1_TX            (6) //PA06
#define PA_UART1_RX            (7) //PA07

#define PA_POWER_ADC           (11) //PA11

/// BLE Configure (Single or Multi-Connections)
#define BLE_NB_SLAVE           (1)
#define BLE_MTU                (0x200)

#define BLE_ADDR               {0x00, 0x09, 0x08, 0x23, 0x20, 0xD2}
#define BLE_DEV_NAME           "ETC-"
#define BLE_DEV_ICON           0x0000

#define APP_ADV_INT_MIN        (32) // unit in 0.625ms
#define APP_ADV_INT_MAX        (32) // unit in 0.625ms

/// Profile Configure
#define PRF_DISS               (1)
#define PRF_SESS               (1)

/// Serial Service @see prf_sess.h
#define SES_UUID_128           (0)
#define SES_READ_SUP           (1)

/// Debug Configure
#if (DBG_MODE)
    #define DBG_APP            (0)
    #define DBG_PROC           (1)
    #define DBG_ACTV           (0)
    #define DBG_GAPM           (0)
    #define DBG_GAPC           (0)
    #define DBG_DISS           (0)
    #define DBG_SESS           (1)
#endif

/// Misc Options
#define LED_PLAY               (1)
#define CFG_SLEEP              (0)
#define MDK_HOUR  ((__TIME__[0]-'0')*10 + __TIME__[1]-'0')
#define MDK_MIN   ((__TIME__[3]-'0')*10 + __TIME__[4]-'0')
//#define FIRM_VERTION           ((MDK_HOUR << 8) | MDK_MIN)
#define FIRM_VERTION           (0x3816)

#define ETC_TEST               (1)

#endif  //_APP_CFG_H_
