/**
 ****************************************************************************************
 *
 * @file proc.c
 *
 * @brief UART command processor for BLE HID device.
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "bledef.h"
#include "drvs.h"

#include "app.h"
#include "uartRb.h"
#include "prf_bass.h"
#include "hid_desc.h"

#if (DBG_PROC)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, (int)__LINE__, ##__VA_ARGS__)
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
    CMD_KB_REP        = 0xC0,
    CMD_MEDIA_REP,
    CMD_SYSTEM_REP,

    CMD_MOUSE_REP     = 0xD0,
    CMD_MOUSE_BTN,
    CMD_MOUSE_X,
    CMD_MOUSE_Y,
    CMD_MOUSE_WHEEL,

    CMD_CONN_INTERVAL = 0xE0,
    CMD_SVC_CHG,

    CMD_BATT_LVL      = 0xF0,
    CMD_BATT_PWR_STA
};

#define CMD_MAX_LEN             20
#define NULL_CNT                20
#define MOUSE_MOVE_DEFAULT_DX   10     ///< Default X movement for simple move command

/// HID keyboard report length
#define KB_RPT_LEN              8
/// HID mouse report length
#define MOUSE_RPT_LEN           6
/// Mouse report field indices
#define MOUSE_BTN_IDX           0
#define MOUSE_X_LSB_IDX         1
#define MOUSE_X_MSB_IDX         2
#define MOUSE_Y_LSB_IDX         3
#define MOUSE_Y_MSB_IDX         4
#define MOUSE_WHEEL_IDX         5

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

    len = uart1Rb_Read(&buff[buff_len], CMD_MAX_LEN - buff_len);
    if (len > 0)
    {
        buff_len += len;
        if (buff_len < CMD_MAX_LEN)
        {
            return; // wait for more data
        }
    }
    else
    {
        if ((buff_len > 0) && (null_cnt++ > NULL_CNT))
        {
            null_cnt = 0;
        }
        else
        {
            return; // wait again
        }
    }

    if (buff_len == 0)
        return;

    if (app_state_get() == APP_CONNECTED)
    {
        switch (buff[0])
        {
            case CMD_KB_REP:
            {
                DEBUG("key_code:0x%02x", buff[1]);
                uint8_t kyebd_report[KB_RPT_LEN] = {0x00, 0x00, buff[1], 0x00, 0x00, 0x00, 0x00, 0x00};
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
                if (buff_len < 2) break;
                DEBUG("CMD_SYSTEM_REP:%x", buff[1]);
                system_report_send(app_env.curidx, &buff[1]);
                system_report_send(app_env.curidx, NULL);
            } break;

            case CMD_MOUSE_REP:
            {
                // Simple move X with default offset
                buff[0] = CMD_MOUSE_X;
                buff[1] = MOUSE_MOVE_DEFAULT_DX;
            }
            /* fallthrough */
            case CMD_MOUSE_BTN:
            case CMD_MOUSE_X:
            case CMD_MOUSE_Y:
            case CMD_MOUSE_WHEEL:
            {
                uint8_t mouse_data[MOUSE_RPT_LEN] = {0};
                uint8_t idx = buff[0] - CMD_MOUSE_BTN;

                // Calculate field index: BTN=0, X=1, Y=3, WHEEL=5
                if (idx > 0)
                    idx = (idx * 2) - 1;

                if (idx < MOUSE_RPT_LEN)
                {
                    mouse_data[idx] = buff[1];
                    DEBUG("MOUSE(1Btn,2X,2Y,1Wheel)[%d]:0x%02x", idx, buff[1]);
                    mouse_report_send(app_env.curidx, mouse_data);
                    mouse_report_send(app_env.curidx, NULL);
                }
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
                if (buff_len >= 4)
                {
                    struct gapc_conn_param conn_pref =
                    {
                        .intv_min = buff[1],
                        .intv_max = buff[2],
                        .latency  = buff[3],
                        .time_out = 300,
                    };

                    DEBUG("intvn:%d, intvm:%d, lat:%d",
                          conn_pref.intv_min, conn_pref.intv_max, conn_pref.latency);
                    gapc_update_param(app_env.curidx, &conn_pref);
                }
            } break;

            case CMD_SVC_CHG:
            {
                DEBUG("service change");
            } break;

            default:
                break;
        }
    }

    buff_len = 0;
}

void user_procedure(void)
{
    uart_proc();
}
