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
#include "drvs.h"

#include "fcc.h"
#include "uartRb.h"
#include "sftmr.h"

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

#define BLE_MAX_LEN       20
#define NULL_CNT          20
#define HOPPING_INV       _MS(200)
#define BLE_FREQ_CHAN_NUM 40    /**< BLE frequency channels (2402-2480 MHz) */
#define TMR_ACTIVE_FLAG   0x80U /**< Bitmask flag for active hopping timer */
#define XOSC_CAP_MASK     0x3FU /**< 6-bit mask for XOSC16M CAP trim */

enum uart_cmd
{
    CMD_FCC_START     = 0xA0,
    CMD_FCC_STOP,

    CMD_FCC_TX_CARR   = 0xB0,
    CMD_FCC_RX_CARR,

    CMD_FCC_TX_MOD    = 0xC0,
    CMD_FCC_RX_MOD,

    CMD_FCC_TX_HOP    = 0xD0,

    CMD_SET_XOSC16_TR = 0xE0,
    CMD_GET_XOSC16_TR,
};

static uint8_t buff[BLE_MAX_LEN];
static uint16_t buff_len = 0;

volatile uint8_t g_hopping_idx = 0;
volatile uint8_t g_hopping_timer_id = 0;

/**
 ****************************************************************************************
 * @brief Hopping timer callback - cycles through BLE channels.
 *
 * @param[in] id    Timer ID (unused)
 *
 * @return Next timer interval in ticks
 ****************************************************************************************
 */
static tmr_tk_t hopping_timer(uint8_t id)
{
    (void)id;

    if (g_hopping_idx >= BLE_FREQ_CHAN_NUM)
    {
        g_hopping_idx = 0;
    }

    fcc_tx_carr(g_hopping_idx);

    ++g_hopping_idx;

    return HOPPING_INV;
}

/**
 ****************************************************************************************
 * @brief Start frequency hopping mode if not already active.
 ****************************************************************************************
 */
static void hopping_mode(void)
{
    if ((g_hopping_timer_id & TMR_ACTIVE_FLAG) == 0)
    {
        uint8_t id = sftmr_start(HOPPING_INV, hopping_timer);

        if (id != 0)
        {
            g_hopping_timer_id = id | TMR_ACTIVE_FLAG;
        }
    }
}
/*
 * FUNCTIONS
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief Process UART data and dispatch FCC commands.
 ****************************************************************************************
 */
static void data_proc(void)
{
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
            null_cnt = 0;
        }
        else
        {
            return; // wait again
        }
    }

    if ((buff[0] != CMD_FCC_TX_HOP) && (g_hopping_timer_id != 0))
    {
        g_hopping_timer_id &= ~TMR_ACTIVE_FLAG;
        sftmr_clear(g_hopping_timer_id);
        g_hopping_timer_id = 0;
    }

    switch (buff[0])
    {
        case CMD_FCC_START:
        {
            DEBUG("fcc_start");
            fcc_init();
        } break;

        case CMD_FCC_STOP:
        {
            DEBUG("fcc_stop");
            fcc_stop();
        } break;

        case CMD_FCC_TX_CARR:
        {
            DEBUG("fcc_tx_carr");
            if ((buff_len > 1) && (buff[1] < BLE_FREQ_CHAN_NUM))
            {
                fcc_tx_carr(buff[1]);
            }
        } break;

        case CMD_FCC_RX_CARR:
        {
            DEBUG("fcc_rx_carr");
            if ((buff_len > 1) && (buff[1] < BLE_FREQ_CHAN_NUM))
            {
                fcc_rx_carr(buff[1]);
            }
        } break;

        case CMD_FCC_TX_MOD:
        {
            DEBUG("fcc_tx_mod");
            if ((buff_len > 1) && (buff[1] < BLE_FREQ_CHAN_NUM))
            {
                fcc_tx_mod(buff[1]);
            }
        } break;

        case CMD_FCC_RX_MOD:
        {
            DEBUG("fcc_rx_mod");
            if ((buff_len > 1) && (buff[1] < BLE_FREQ_CHAN_NUM))
            {
                fcc_rx_mod(buff[1]);
            }
        } break;

        case CMD_FCC_TX_HOP:
        {
            DEBUG("hopping_mode");
            g_hopping_idx = 0;
            hopping_mode();
        } break;

        case CMD_SET_XOSC16_TR:
        {
            if (buff_len > 1)
            {
                DEBUG("SET_XOSC_TR:0x%02x", buff[1]);
                APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR = (buff[1] & XOSC_CAP_MASK);
            }
        } break;

        case CMD_GET_XOSC16_TR:
        {
            DEBUG("GET_XOSC_TR:0x%02x", APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR);
        } break;

        default:
        {

        } break;
    }

    buff_len = 0;
}

/**
 ****************************************************************************************
 * @brief Main loop entry point for UART command processing.
 ****************************************************************************************
 */
void user_procedure(void)
{
    data_proc();
}
