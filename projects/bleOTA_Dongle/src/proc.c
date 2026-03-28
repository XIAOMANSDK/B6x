/**
 ****************************************************************************************
 *
 * @file proc.c
 *
 * @brief user procedure.
 *
 ****************************************************************************************
 */

#include "drvs.h"
#include "bledef.h"

#include "app.h"
#include "uartRb.h"
#include "sftmr.h"
#include "app_user.h"

#if (DBG_PROC)
#include "dbg.h"
#define DEBUG(format, ...) debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif

/*
 * DEFINES
 ****************************************************************************************
 */

#define BLE_MAX_LEN 64 //(BLE_MTU - 3)

static uint8_t  buff[BLE_MAX_LEN];
static uint16_t buff_len = 0;
#define NULL_CNT 20

#define SCAN_NAME_MAX 29 // 31 - 2

#define SWAP_BT_ORDER (0)

struct cmd_env_tag
{
    uint8_t scan_name[30];
    uint8_t auto_conn;
    uint8_t scan_duration;
} __attribute__((packed));

struct cmd_env_tag cmd_env;

enum
{
    CMD_SET_SCAN_NAME = 0xA5, // +len + name_len + name
    CMD_SET_SCAN_DURATION,    // +len(1B) + duration(1)
    CMD_SET_CONN_ADDR,        // +len(7) + addr(6) + addr_type(1)
    CMD_SET_AUTO,             // +len(1) + 0/1
    CMD_SET_DISCONN,          // +len(0)
};

tmr_tk_t uart_rx_to;
#define UART_RX_TO             _MS(100)
/// Tick timeout arrived
#define TMR_TICK_OUT(now, out) ((tmr_tk_t)((now) - (out)) <= SFTMR_DELAY_MAX)

uint8_t *get_scan_name(uint8_t *len)
{
    *len = cmd_env.scan_name[0];
    return cmd_env.scan_name + 1;
}

uint8_t get_auto_conn(void)
{
    return cmd_env.auto_conn;
}

uint8_t get_scan_duration(void)
{
    return cmd_env.scan_duration;
}

void user_init(void)
{
    memset(cmd_env.scan_name, 0x00, sizeof(cmd_env));

    cmd_env.scan_name[0] = sizeof(SCAN_ADV_NAME) - 1;
    memcpy(cmd_env.scan_name + 1, SCAN_ADV_NAME, cmd_env.scan_name[0]);
    //    cmd_env.auto_conn = 1;
    cmd_env.scan_duration = 50;

    uart_rx_to = sftmr_tick() + UART_RX_TO;
}

/*
 * FUNCTIONS
 ****************************************************************************************
 */
#if (SWAP_BT_ORDER)
__INLINE__ void co_bswap(uint8_t *p_val_out, const uint8_t *p_val_in, uint16_t len)
{
    while (len > 0)
    {
        len--;
        *p_val_out = p_val_in[len];
        p_val_out++;
    }
}
#endif

void user_procedure(void)
{
    uint16_t       len;
    static uint8_t null_cnt = 0;
    bool           finsh    = false;

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
            null_cnt = 0;
        }
        else
        {
            return; // wait again
        }
    }

    if (buff_len > 1)
    {
        debugHex(buff, buff_len);

        uint8_t cmd = buff[0];
        len         = buff[1];
        switch (cmd)
        {
            case CMD_SET_SCAN_NAME:
            {
                if (buff_len > 2)
                {
                    uint8_t name_len = buff[2];
                    if (buff_len >= name_len + 2)
                    {
                        memcpy(cmd_env.scan_name, buff + 2, name_len + 1);
                        finsh = true;
                        DEBUG("update scan name");
                        app_scan_action(ACTV_START);
                    }
                }
            }
            break;

            case CMD_SET_SCAN_DURATION:
            {
                if ((buff_len >= 3) && (len == 1))
                {
                    cmd_env.scan_duration = buff[2];

                    finsh = true;
                    DEBUG("update scan duration:%d", cmd_env.scan_duration);
                    app_scan_action(ACTV_START);
                }
            }
            break;

            case CMD_SET_CONN_ADDR:
            {
                if ((buff_len >= 9) && (len == 7))
                {
                    uint8_t local_bt[7];
#if (SWAP_BT_ORDER)
                    co_bswap(local_bt, buff + 2, 6);
                    local_bt[6] = buff[8];
#else
                    memcpy(local_bt, buff + 2, 7);
#endif

                    cmd_env.auto_conn = 0;
                    app_start_initiating((const struct gap_bdaddr *)local_bt);
                    finsh = true;
                    DEBUG("update conn_addr");
                }
            }
            break;

            case CMD_SET_AUTO:
            {
                if (buff_len > 2)
                {
                    cmd_env.auto_conn = buff[2];

                    init_timer_start();
                    finsh = true;
                }
            }
            break;

            case CMD_SET_DISCONN:
            {
                if (app_state_get() >= APP_CONNECTED)
                {
                    gapc_disconnect(app_env.curidx);
                }
                cmd_env.auto_conn = 0;
                finsh             = true;
            }
            break;

            default:
            {
                finsh = true;
            }
            break;
        }

        if ((finsh) || (TMR_TICK_OUT(sftmr_tick(), uart_rx_to)))
        {
            uart_rx_to = sftmr_tick() + UART_RX_TO;
            buff_len   = 0;
        }
    }
}
