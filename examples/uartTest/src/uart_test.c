/**
 ****************************************************************************************
 *
 * @file uart_test.c
 *
 * @brief UART loopback test example
 *
 ****************************************************************************************
 */

#include "drvs.h"
#include "dbg.h"


/*
 * DEFINES
 ****************************************************************************************
 */

#define TEST_PORT          UART1_PORT
#define TEST_BAUD          BRR_115200
#define TEST_LCRS          LCR_BITS_DFLT

#define PA_UART_TX         (6)
#define PA_UART_RX         (7)
#define PA_UART_RTS        (14)
#define PA_UART_CTS        (15)


/*
 * FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief UART initialization
 ****************************************************************************************
 */
static void uartInit(void)
{
    #if !((DBG_MODE == 1) && (TEST_PORT == UART1_PORT))
    uart_init(TEST_PORT, PA_UART_TX, PA_UART_RX);
    uart_conf(TEST_PORT, TEST_BAUD, TEST_LCRS);

    #if (UART_IRQ_MODE)
    uart_fctl(TEST_PORT, FCR_FIFOEN_BIT | FCR_RXTL_8BYTE, 20, UART_IR_RXRD_BIT | UART_IR_RTO_BIT);
    #endif

    #if (UART_RTS_CTRL)
    uart_hwfc(TEST_PORT, PA_UART_RTS, PA_UART_CTS);
    #endif
    #endif
}

/**
 ****************************************************************************************
 * @brief UART loopback test: receive and echo back
 ****************************************************************************************
 */
void uartTest(void)
{
    uint8_t rx_data;

    uartInit();

    while (1)
    {
        rx_data = uart_getc(TEST_PORT);
        uart_putc(TEST_PORT, rx_data);
    }
}
