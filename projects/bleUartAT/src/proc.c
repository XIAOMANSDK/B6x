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
#include "atcmd.h"

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

#define CMD_MAX_LEN           64
#define NULL_CNT              2000

static uint8_t buff[CMD_MAX_LEN];
static uint16_t buff_len = 0;


/*
 * FUNCTIONS
 ****************************************************************************************
 */

/// Override - Callback on received data from peer device
void sess_cb_rxd(uint8_t conidx, uint16_t len, const uint8_t *data)
{
    uart_send(UART1_PORT, len, data);
}

/// Uart Data procedure
void user_procedure(void)
{
    static uint16_t null_cnt = 0;
    uint16_t len;
    bool finish = true;
    
    len = uart1Rb_Read(&buff[buff_len], CMD_MAX_LEN - buff_len);
    if (len > 0)
    {
        buff_len += len;
        
        if (buff_len < CMD_MAX_LEN)
        {   
            null_cnt = 0;         
            return; // wait full
        }
    }
    else
    {
        if ((buff_len > 0) && (null_cnt++ > NULL_CNT))
        {
            finish = true;
            null_cnt = 0;
        }
        else
        {
            return; // wait again
        }
    }
    
    if (app_state_get() == APP_CONNECTED)
    {
        atBleTx(buff, buff_len);
    } 
    else
    {
        atProc(buff, buff_len);    
    }
    

    if (finish)
    {
        buff_len = 0;
    }
}
