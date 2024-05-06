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
#include "usbd_hid.h"

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
    uint8_t state = 0xFF;
    uint8_t usb_rpt[8] = {0};
    
    if (data[0] == 0xA0)
    {
        memcpy(usb_rpt, data + 1, len > 8 ? 8 : len - 1);
        state = usbd_hid_send_report(KEYBD_IN_EP, 8, usb_rpt); // key code
        memset(usb_rpt, 0x00, 8);
        state = usbd_hid_send_report(KEYBD_IN_EP, 8, usb_rpt); // release
    }
    
    if (data[0] == 0xA1)
    {
        memcpy(usb_rpt, data + 1, len > 4 ? 4 : len - 1);
        state = usbd_hid_send_report(MOUSE_IN_EP, 4, data + 1);
    }
    
    DEBUG("USBD(sta:%x)", state);
    debugHex(data, len);
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
            uint8_t mouse_report[4] = {0};
            mouse_report[1] = buff[1];
            uint8_t state = usbd_hid_send_report(MOUSE_IN_EP, 4, mouse_report);

            DEBUG("USBD(sta:%x)", state);
            debugHex(buff, buff_len);
        } break;
        
        default:
        {
        } break;
    }

    buff_len = 0;
}
