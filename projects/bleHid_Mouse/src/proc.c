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
#include "mouse.h"
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

#if (UART_CMD)
enum uart_cmd
{
    CMD_MOUSE_REP     = 0xD0,

    CMD_CONN_INTERVAL = 0xE0,

    CMD_BATT_LVL      = 0xF0,
    CMD_BATT_PWR_STA
};

#define CMD_MAX_LEN             20
#define NULL_CNT                20

/// Connection supervision timeout (unit in 10ms)
#define CONN_SUPERVISION_TO     300

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
            case CMD_MOUSE_REP:
            {
                DEBUG("MOUSE");
                uint8_t mouse_data[MOUSE_RPT_LEN] = {0, MOUSE_MOVE_DX, 0, 0, 0, 0};
                mouse_report_send(app_env.curidx, mouse_data);
                mouse_report_send(app_env.curidx, NULL);
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
                        .time_out = CONN_SUPERVISION_TO,
                    };

                    DEBUG("intvn:%d, intvm:%d, lat:%d", conn_pref.intv_min, conn_pref.intv_max, conn_pref.latency);
                    gapc_update_param(app_env.curidx, &conn_pref);
                }
            } break;

            default:
            {
            } break;
        }
    }

    buff_len = 0;
}
#endif //(UART_CMD)

#if (CFG_SLEEP)
static void sleep_proc(void)
{
    //uint8_t lpsta = ble_sleep(BLE_SLP_TWOSC, BLE_SLP_DURMAX);
    #if (CFG_POWEROFF)
    uint32_t slpdur = ble_slpdur_get();

    if (slpdur > 200)
    {
        // Core enter poweroff mode
        if (ble_sleep(130, slpdur) == BLE_IN_SLEEP)
        {
            core_pwroff(CFG_WKUP_BLE_EN | WKUP_IO_LATCH_N_BIT);
        }
    }
    else if (slpdur > 100)
    {
        // Core enter deepsleep mode
        if (ble_sleep(64, slpdur) == BLE_IN_SLEEP)
        {
            core_sleep(CFG_WKUP_BLE_EN);
        }
    }
    #else //!(CFG_POWEROFF)
    uint8_t lpsta = ble_sleep(BLE_SLP_TWOSC, BLE_SLP_DURMAX);

    if (lpsta == BLE_IN_SLEEP)
    {
        uint16_t lpret = core_sleep(CFG_WKUP_BLE_EN);
        DEBUG("ble sta:%d, wksrc:%X", lpsta, lpret);
    }
    else
    {
        DEBUG("ble sta:%d", lpsta);
    }
    #endif //(CFG_POWEROFF)
}
#endif //(CFG_SLEEP)

void user_procedure(void)
{
    #if (CFG_SLEEP)
    sleep_proc();
    #endif

    #if (UART_CMD)
    uart_proc();
    #endif //(UART_CMD)

    mouse_scan();
}
