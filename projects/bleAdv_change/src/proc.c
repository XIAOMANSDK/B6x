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

#define BLE_MAX_LEN           20
#define NULL_CNT              20

static uint8_t buff[BLE_MAX_LEN];
static uint16_t buff_len = 0;


/*
 * FUNCTIONS
 ****************************************************************************************
 */

#if !(DBG_SESS)
/// Override - Callback on received data from peer device
void sess_cb_rxd(uint8_t conidx, uint16_t len, const uint8_t *data)
{
    uart_send(UART1_PORT, len, data);
}
#endif //!(DBG_SESS)

/// Uart Data procedure
static void data_proc(void)
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

    if (app_state_get() == APP_CONNECTED)
    {
        if (buff[0] == 0xAA)
        {
            DEBUG("GAP Disc!\r\n");
            gapc_disconnect(app_env.curidx);
            buff_len = 0;
        }
        else if (sess_txd_send(app_env.curidx, buff_len, buff) == LE_SUCCESS)
        {
            debugHex(buff, buff_len);
            buff_len = 0;
        }
    }
    else
    {
        // goto reset
        if (buff[0] == 0xAA)
        {
            DEBUG("GAP Reset!\r\n");
            gapm_reset();
        }

        buff_len = 0;
    }
}

#if (CFG_SLEEP)
static void sleep_proc(void)
{
    uint8_t lpsta = ble_sleep(BLE_SLP_TWOSC, BLE_SLP_DURMAX);

    if (lpsta == BLE_IN_SLEEP)
    {
        uint16_t lpret = core_sleep(CFG_WKUP_BLE_EN);
        //DEBUG("ble sta:%d, wksrc:%X", lpsta, lpret);
    }
    else
    {
        //DEBUG("ble sta:%d", lpsta);
    }
}
#endif //(CFG_SLEEP)

void user_procedure(void)
{
    #if (CFG_SLEEP)
    sleep_proc();
    #endif //(CFG_SLEEP)

    data_proc();
}
