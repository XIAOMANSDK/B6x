/**
 ****************************************************************************************
 *
 * @file uart_test.c
 *
 * @brief Demo of uart usage.
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "drvs.h"
#include "dbg.h"


/*
 * DEFINES
 ****************************************************************************************
 */

/// Uart Port and Params
#define TEST_PORT          UART1_PORT
#define TEST_BAUD          BRR_DIV(115200, 16M)
#define TEST_LCRS          LCR_BITS(8, 1, none)

/// Avoid conflict GPIOs of UART_DBG if diff port
#define PA_UART_TX         (6)
#define PA_UART_RX         (7)
#define PA_UART_RTS        (14)
#define PA_UART_CTS        (15)


/*
 * FUNCTIONS
 ****************************************************************************************
 */

static void uartInit(void)
{
    #if !((DBG_MODE == 1) && (TEST_PORT == UART1_PORT))
    uart_init(TEST_PORT, PA_UART_TX, PA_UART_RX);
    uart_conf(TEST_PORT, TEST_BAUD, TEST_LCRS);
    #endif
    
    #if (UART_IRQ_MODE)
    uart_fctl(TEST_PORT, FCR_FIFOEN_BIT | FCR_RXTL_8BYTE, 20, UART_IR_RXRD_BIT | UART_IR_RTO_BIT);
    #endif
    
    #if (UART_RTS_CTRL)
    uart_hwfc(TEST_PORT, PA_UART_RTS, PA_UART_CTS);
    #endif
}

void uartTest(void)
{
    uint8_t rx_data;
    
    uartInit();
    
    while (1)
    {
        // loopback Test
        rx_data = uart_getc(TEST_PORT);
        uart_putc(TEST_PORT, rx_data);
    }
}
