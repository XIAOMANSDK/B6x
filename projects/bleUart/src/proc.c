/**
 ****************************************************************************************
 *
 * @file proc.c
 *
 * @brief User procedure - UART to BLE transparent data forwarding with command
 *        parsing for disconnect, reset, and speed-test modes.
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
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, (int)__LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif


/*
 * DEFINES
 ****************************************************************************************
 */

/** BLE_MAX_LEN = MTU - 3 (1 byte ATT opcode + 2 bytes ATT handle) */
#define BLE_MAX_LEN                 (BLE_MTU - 3)

/** Idle poll count threshold before flushing partial UART data */
#define UART_IDLE_FLUSH_THRESHOLD   (20)

/** Command byte: disconnect BLE link */
#define CMD_DISCONNECT              (0xAA)

/** Command byte: enter speed test mode */
#define CMD_SPEED_TEST              ('S')


/*
 * VARIABLES
 ****************************************************************************************
 */

static uint8_t  g_uart_rx_buf[BLE_MAX_LEN];
static uint16_t g_uart_rx_len = 0;
static bool     g_speed_test = false;


/*
 * FUNCTIONS
 ****************************************************************************************
 */

#if !(DBG_SESS)
/**
 ****************************************************************************************
 * @brief Callback on data received from BLE peer. Forwards data to UART1.
 *
 * @param[in] conidx   Connection index
 * @param[in] len      Data length
 * @param[in] data     Pointer to received data
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief BLE session data received callback
 ****************************************************************************************
 */
void sess_cb_rxd(uint8_t conidx, uint16_t len, const uint8_t *data)
{
    (void)conidx;
    uart_send(UART1_PORT, len, data);
}
#endif /* !(DBG_SESS) */

/**
 ****************************************************************************************
 * @brief Process UART data: accumulate into buffer and forward to BLE.
 *
 *        Reads UART ring buffer into g_uart_rx_buf. When the buffer is full or
 *        UART has been idle for UART_IDLE_FLUSH_THRESHOLD polls, the data is
 *        sent over BLE via sess_txd_send(). Special command bytes:
 *        - CMD_DISCONNECT: disconnect BLE or reset stack
 *        - CMD_SPEED_TEST: enable speed test mode
 ****************************************************************************************
 */
static void data_proc(void)
{
    static uint8_t idle_poll_cnt = 0;
    uint16_t read_len;

    read_len = uart1Rb_Read(&g_uart_rx_buf[g_uart_rx_len], BLE_MAX_LEN - g_uart_rx_len);
    if (read_len > 0)
    {
        g_uart_rx_len += read_len;
        if (g_uart_rx_len < BLE_MAX_LEN)
        {
            return;
        }
    }
    else
    {
        if ((g_uart_rx_len > 0) && (idle_poll_cnt++ > UART_IDLE_FLUSH_THRESHOLD))
        {
            idle_poll_cnt = 0;
        }
        else
        {
            return;
        }
    }

    if (app_state_get() == APP_CONNECTED)
    {
        if (g_uart_rx_buf[0] == CMD_DISCONNECT)
        {
            g_speed_test = false;
            DEBUG("GAP Disc!");
            gapc_disconnect(app_env.curidx);
            g_uart_rx_len = 0;
        }
        else if (g_uart_rx_buf[0] == CMD_SPEED_TEST)
        {
            g_speed_test = true;
            g_uart_rx_len = 0;
        }
        else if (sess_txd_send(app_env.curidx, g_uart_rx_len, g_uart_rx_buf) == LE_SUCCESS)
        {
            debugHex(g_uart_rx_buf, g_uart_rx_len);
            g_uart_rx_len = 0;
            idle_poll_cnt = 0;
        }
    }
    else
    {
        if (g_uart_rx_buf[0] == CMD_DISCONNECT)
        {
            DEBUG("GAP Reset!");
            gapm_reset();
        }

        g_uart_rx_len = 0;
    }
}

#if (CFG_SLEEP)
/**
 ****************************************************************************************
 * @brief Enter low-power sleep when BLE stack permits.
 ****************************************************************************************
 */
static void sleep_proc(void)
{
    uint8_t lpsta = ble_sleep(BLE_SLP_TWOSC, BLE_SLP_DURMAX);

    if (lpsta == BLE_IN_SLEEP)
    {
        (void)core_sleep(CFG_WKUP_BLE_EN);
    }
}
#endif /* (CFG_SLEEP) */

/**
 ****************************************************************************************
 * @brief Main user procedure called every loop iteration.
 *        Handles sleep, UART-to-BLE data forwarding, and speed test.
 ****************************************************************************************
 */
void user_procedure(void)
{
    #if (CFG_SLEEP)
    sleep_proc();
    #endif

    data_proc();

    if ((app_state_get() == APP_CONNECTED) && (g_speed_test))
    {
        if (sess_txd_send(app_env.curidx, BLE_MAX_LEN, g_uart_rx_buf) == LE_SUCCESS)
        {
            g_uart_rx_len = 0;
        }
    }
}
