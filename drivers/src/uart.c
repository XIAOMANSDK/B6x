/**
 ****************************************************************************************
 *
 * @file uart.c
 *
 * @brief UART Driver
 *
 ****************************************************************************************
 */
#include "reg_uart.h"
#include "reg_gpio.h"
#include "uart.h"
#include "iopad.h"
#include "rcc.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Get UARTx Pointer, 'port' : 0-UART1, 1-UART2
#if defined(UART1_BASE)
#define UART_PTR(port)         ((UART_TypeDef *)(UART1_BASE + (port) * 0x1000))
#else
#define UART_PTR(port)         ((port == 0) ? UART1 : UART2)
#endif


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void uart_init(uint8_t port, uint8_t io_tx, uint8_t io_rx)
{    
    iom_ctrl(io_rx, IOM_SEL_CSC | IOM_PULLUP | IOM_INPUT);
    iom_ctrl(io_tx, IOM_SEL_CSC | IOM_PULLUP | IOM_DRV_LVL1);   
    
    //iocsc_uart(port, io_tx, io_rx);
    csc_output(io_tx, CSC_UART1_TXD + port * 2);
    csc_input(io_rx,  CSC_UART1_RXD + port * 2);
    
    // uart_clk_en rst_req
    RCC_APBCLK_EN(1 << (RCC_UART1_CLKEN_RUN_POS + port));
    RCC_APBRST_REQ(1 << (RCC_UART1_RSTREQ_POS + port));
    
    uint8_t delay_cnt = 0x09;    
    // Delay for Uart busy
    while (delay_cnt--)  //220ns/cnt(64Mhz test need delay 906ns)  --20231204 WHL
    {
        if ((GPIO->PIN >> io_rx) & 0x01)
            return;
    }
}

void uart_hwfc(uint8_t port, uint8_t io_rts, uint8_t io_cts)
{
    UART_TypeDef* uart = UART_PTR(port);
    
    //iocsc_uart_hwfc(port, io_rts, io_cts);
    csc_output(io_rts, CSC_UART1_RTS + port);
    csc_input(io_cts,  CSC_UART1_CTS + port);
    
    iom_ctrl(io_rts, IOM_SEL_CSC | IOM_DRV_LVL1 | IOM_PULLUP);
    iom_ctrl(io_cts, IOM_SEL_CSC | IOM_PULLUP   | IOM_INPUT);
    
    // enable auto ctrl bit[3:2] (.AFCEN=1, .RTSCTRL=1)
    uart->MCR.Word |= 0x0C;
}

void uart_conf(uint8_t port, uint16_t cfg_BRR, uint16_t cfg_LCR)
{
    UART_TypeDef* uart = UART_PTR(port);
    
    // clear en
    uart->LCR.Word   = 0;
    
    // update BaudRate
    uart->LCR.BRWEN  = 1; 
    uart->BRR        = cfg_BRR; // (rcc_sysclk_get() + (baud >> 1)) / baud
    uart->LCR.BRWEN  = 0;
    
    // enable fifo mode, reset
    uart->FCR.Word |= 0x07; // FIFOEN RFRST TFRST
    
    // config params, enable
    uart->LCR.Word = cfg_LCR | LCR_RXEN_BIT;
}

void uart_fctl(uint8_t port, uint8_t fifo_ctl, uint16_t bits_rto, uint16_t intr_en)
{
    UART_TypeDef* uart = UART_PTR(port);
    
    uart->FCR.Word  = fifo_ctl; // | FCR_FIFOEN_BIT; // set bit[7:0]
    uart->RTOR.Word = bits_rto; // .RTO bit[23:0]
    uart->LCR.RTOEN = (bits_rto) ? 1 : 0;
    
    uart->IDR.Word  = UART_IR_ALL_MSK;
    uart->IER.Word  = intr_en;
}

void uart_mctl(uint8_t port, uint8_t dma)
{
    UART_TypeDef *uart = UART_PTR(port);
    
    uart->MCR.DMAEN = dma;
    uart->LCR.RTO_SEL = dma;
}

void uart_putc(uint8_t port, uint8_t ch)
{
    UART_TypeDef *uart = UART_PTR(port);

    while (!(uart->SR.TBEM));
    uart->TBR = ch;
}

uint8_t uart_getc(uint8_t port)
{
    UART_TypeDef *uart = UART_PTR(port);

    while (!(uart->SR.DR));
    return (uint8_t)(uart->RBR);
}

void uart_wait(uint8_t port)
{
    UART_TypeDef *uart = UART_PTR(port);
    
    while (!(uart->SR.TBEM)); // wait tx finish
    while (uart->SR.BUSY);    // wait idle state
}

void uart_send(uint8_t port, uint16_t len, const uint8_t *data)
{
    UART_TypeDef *uart = UART_PTR(port);

    while (len--)
    {
        while (!(uart->SR.TBEM));
        uart->TBR = *data++;
    }
    
    while (!(uart->SR.TBEM)); // wait tx finish
    while (uart->SR.BUSY);    // wait idle state
}
