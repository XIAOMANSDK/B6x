/**
 ****************************************************************************************
 *
 * @file proc.c
 *
 * @brief User procedure - UART data accumulation and routing to USB CDC or BLE
 *        Serial Service based on command byte prefix.
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "bledef.h"
#include "drvs.h"

#include "app.h"
#include "prf_sess.h"
#include "uartRb.h"
#include "usbd_cdc.h"

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

/** Command byte: route data to USB CDC endpoint */
#define CMD_USBD                (0xA0)

/** Command byte: route data to BLE Serial Service */
#define CMD_BLE_SESS            (0xB0)

/** CDC bulk IN endpoint address (must match cdc_uart.c descriptor) */
#define CDC0_IN_EP              (0x81)

/**
 * BLE maximum payload per packet.
 * Fixed 20 bytes for BLE 4.0 compatibility (default MTU = 23, minus 3 ATT header).
 */
#define BLE_MAX_PAYLOAD         (20)

/** Idle poll count threshold before flushing partial UART data */
#define UART_IDLE_FLUSH_THRESHOLD   (20)


/*
 * VARIABLES
 ****************************************************************************************
 */

static uint8_t  g_uart_rx_buf[BLE_MAX_PAYLOAD];
static uint16_t g_uart_rx_len = 0;


/*
 * FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Callback on data received from BLE peer. Forwards to UART1 and USB CDC.
 *
 * @param[in] conidx   Connection index
 * @param[in] len      Data length
 * @param[in] data     Pointer to received data
 ****************************************************************************************
 */
void sess_cb_rxd(uint8_t conidx, uint16_t len, const uint8_t *data)
{
    (void)conidx;
    uart_send(UART1_PORT, len, data);
    (void)usbd_cdc_ep_send(CDC0_IN_EP, len, data);
}

/**
 ****************************************************************************************
 * @brief Main user procedure called every loop iteration.
 *
 *        Reads UART ring buffer into g_uart_rx_buf. When the buffer is full or
 *        UART has been idle for UART_IDLE_FLUSH_THRESHOLD polls, data is routed
 *        based on the first byte:
 *        - CMD_USBD (0xA0): send to USB CDC endpoint
 *        - CMD_BLE_SESS (0xB0): send to BLE Serial Service (if connected)
 *        - Other: discard
 ****************************************************************************************
 */
void user_procedure(void)
{
    static uint8_t idle_poll_cnt = 0;
    uint16_t read_len;

    read_len = uart1Rb_Read(&g_uart_rx_buf[g_uart_rx_len], BLE_MAX_PAYLOAD - g_uart_rx_len);
    if (read_len > 0)
    {
        g_uart_rx_len += read_len;
        if (g_uart_rx_len < BLE_MAX_PAYLOAD)
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

    switch (g_uart_rx_buf[0])
    {
        case CMD_USBD:
        {
            uint8_t status = usbd_cdc_ep_send(CDC0_IN_EP, g_uart_rx_len, g_uart_rx_buf);
            DEBUG("USBD(sta:%x)", status);
            (void)status;
            debugHex(g_uart_rx_buf, g_uart_rx_len);
        }
        break;

        case CMD_BLE_SESS:
        {
            if (app_state_get() > APP_READY)
            {
                uint8_t status = sess_txd_send(app_env.curidx, g_uart_rx_len, g_uart_rx_buf);
                DEBUG("BLE_SESS(le_sta:%x)", status);
                (void)status;
                debugHex(g_uart_rx_buf, g_uart_rx_len);
            }
        }
        break;

        default:
            break;
    }

    g_uart_rx_len = 0;
    idle_poll_cnt = 0;
}
