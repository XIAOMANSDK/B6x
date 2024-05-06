/**
 ****************************************************************************************
 *
 * @file proc.c
 *
 * @brief user procedure.
 *
 ****************************************************************************************
 */
#include "regs.h"
#include "drvs.h"

#include "uartRb.h"
#include "gfsk.h"

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
    CMD_SET_XOSC16_TR = 0xA0,
    CMD_GET_XOSC16_TR,
    
    CMD_TX_FREQ       = 0xB0,
};

#define CMD_MAX_LEN 20
#define NULL_CNT 20

static uint8_t buff[CMD_MAX_LEN];
static uint16_t buff_len = 0;

/*
 * FUNCTIONS
 ****************************************************************************************
 */
void uart_proc(void)
{
    static uint8_t null_cnt = 0;
    uint16_t len;
    bool finish = true;
    
    len = uart1Rb_Read(&buff[buff_len], CMD_MAX_LEN - buff_len);
    if (len > 0)
    {
        buff_len += len;
        if (buff_len < CMD_MAX_LEN)
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

    switch(buff[0])
    {
        case CMD_SET_XOSC16_TR:
        {
            if (buff_len > 1)
            {
                DEBUG("SET_XOSC_TR:0x%02x", buff[1]);
                APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR = buff[1];
            }
        } break;
        
        case CMD_GET_XOSC16_TR:
        {
            DEBUG("GET_XOSC_TR:0x%02x", APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR);
        } break;
        
        case CMD_TX_FREQ:
        {
            if (buff_len > 1)
            {
                uint16_t tx_freq_mhz = 2402 + (buff[1] << 1);
                DEBUG("TX_FREQ:%d", tx_freq_mhz);
                rf_gfsk_tx_freq(tx_freq_mhz);
            }
        } break;
        
        default:
        {
        } break;
    }
    
    if (finish)
    {
        buff_len = 0;
    }
}
