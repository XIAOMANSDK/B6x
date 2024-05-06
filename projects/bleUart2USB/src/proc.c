/**
 ****************************************************************************************
 *
 * @file proc.c
 *
 * @brief user procedure.
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "bledef.h"
#include "drvs.h"

#include "app.h"
#include "prf_sess.h"
#include "uartRb.h"
#include "usbd_cdc.h"

#if (DBG_PROC)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif


/*
 * DEFINES
 ****************************************************************************************
 */

enum uart_cmd
{
    CMD_USBD     = 0xA0,
    CMD_BLE_SESS = 0xB0,
};

#define CDC0_IN_EP            0x81
#define BLE_MAX_LEN           20
#define NULL_CNT              20

static uint8_t buff[BLE_MAX_LEN];
static uint16_t buff_len = 0;


/*
 * FUNCTIONS
 ****************************************************************************************
 */

/// Override - Callback on received data from peer device
void sess_cb_rxd(uint8_t conidx, uint16_t len, const uint8_t *data)
{
    uart_send(UART1_PORT, len, data);
    usbd_cdc_ep_send(CDC0_IN_EP, len, data);
}

/// Uart Data procedure
void user_procedure(void)
{
    // Todo Loop-Proc
    static uint8_t null_cnt = 0;
    uint16_t len;

    len = uart1Rb_Read(&buff[buff_len], BLE_MAX_LEN - buff_len);
    if (len > 0)
    {
        buff_len += len;
        if (buff_len < BLE_MAX_LEN)
        {
            return; // wait full
        }
    }
    else
    {
        if ((buff_len > 0) && (null_cnt++ > NULL_CNT))
        {
            //finish = true;
            null_cnt = 0;
        }
        else
        {
            return; // wait again
        }
    }
    
    switch (buff[0])
    {    
        case CMD_USBD:
        {
            uint8_t state = usbd_cdc_ep_send(CDC0_IN_EP, buff_len, buff);

            DEBUG("USBD(sta:%x)", state);
            debugHex(buff, buff_len);
        } break;

        case CMD_BLE_SESS:
        {
            if (app_state_get() > APP_READY)
            {
                uint8_t state = sess_txd_send(app_env.curidx, buff_len, buff);

                DEBUG("BLE_SESS(le_sta:%x, app_sta:%x)", state, app_state_get());
                debugHex(buff, buff_len);
            }
            
        } break;
        
        default:
        {
        } break;
    }

    buff_len = 0;
}
