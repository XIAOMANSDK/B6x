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
#include "keys.h"
#include "uartRb.h"
#include "prf_bass.h"
#include "hid_desc.h"

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

#if (UART_CMD)
enum uart_cmd
{
    CMD_KB_REP        = 0xC0,
    CMD_MEDIA_REP,
    CMD_SYSTEM_REP,
    
    CMD_CONN_INTERVAL = 0xE0,
    
    CMD_BATT_LVL      = 0xF0,
    CMD_BATT_PWR_STA
};

#define CMD_MAX_LEN 20
#define NULL_CNT 20

static uint8_t buff[CMD_MAX_LEN];
static uint16_t buff_len = 0;


/*
 * FUNCTIONS
 ****************************************************************************************
 */

static void uart_proc(void)
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

    if (app_state_get() == APP_CONNECTED)
    {
        switch (buff[0])
        {    
            case CMD_KB_REP:
            {
                DEBUG("key_code:0x%02x", buff[1]);
                uint8_t kyebd_report[] = {0x00, 0x00, buff[1], 0x00, 0x00, 0x00, 0x00, 0x00 }; //6
                keybd_report_send(app_env.curidx, kyebd_report);
                keybd_report_send(app_env.curidx, NULL);
            } break;
            
            case CMD_MEDIA_REP:
            {
                if (buff_len >= 4)
                {
                    DEBUG("MEDIA: %02X %02X %02X", buff[1], buff[2], buff[3]);

                    media_report_send(app_env.curidx, &buff[1]);
                    media_report_send(app_env.curidx, NULL);
                }
            } break;
            
            case CMD_SYSTEM_REP:
            {
                DEBUG("CMD_SYSTEM_REP:%x", buff[1]);
               
                system_report_send(app_env.curidx, &buff[1]);
                system_report_send(app_env.curidx, NULL);
            } break;

            case CMD_BATT_LVL:
            {
                DEBUG("Batt Lvl");
                bass_bat_lvl_update(buff[1]);
            } break;
            
            case CMD_BATT_PWR_STA:
            {
                DEBUG("Batt Power State");
                bass_pwr_sta_update(buff[1]);
            } break;
            
            case CMD_CONN_INTERVAL:
            {
                struct gapc_conn_param conn_pref =
                { 
                    /// Connection interval minimum unit in 1.25ms
                    .intv_min = 6,
                    /// Connection interval maximum unit in 1.25ms
                    .intv_max = 6,
                    /// Slave latency
                    .latency  = 0,
                    /// Connection supervision timeout multiplier unit in 10ms
                    .time_out = 300,
                };
                
                conn_pref.intv_min = buff[1];
                conn_pref.intv_max = buff[2];
                conn_pref.latency  = buff[3];

                DEBUG("intvn:%d, intvm:%d, lat:%d", conn_pref.intv_min, conn_pref.intv_max, conn_pref.latency);
                gapc_update_param(app_env.curidx, &conn_pref);
            } break;
            
            default:
            {
            } break;
        }
    }

    if (finish)
    {
        buff_len = 0;
    }
}
#endif //(UART_CMD)

void user_procedure(void)
{
    #if (UART_CMD)
    uart_proc();
    #endif //(UART_CMD)
    
    keys_scan();
}
