/**
 ****************************************************************************************
 *
 * @file uart1Rb.c
 *
 * @brief Demo of UART1 Interrupt-Mode with RingBuffer. *User should override it*
 *
 ****************************************************************************************
 */

#include <stdint.h>
#include "b6x.h"
#include "uartRb.h"

#if (USE_UART1)  // Remove UARTx_IRQHandler if unused

/*
 * DEFINES
 ****************************************************************************************
 */

#if !defined(UART1_RBUF_SIZE)
    #define UART1_RBUF_SIZE     0x100
#endif

#if !defined(UART1_CONF_LCRS)
    #define UART1_CONF_LCRS     LCR_BITS(8, 1, none) //default
#endif

#if !defined(UART1_CONF_BAUD)
#if (SYS_CLK == 1)
    #define UART1_CONF_BAUD     BRR_DIV(115200, 32M)
#elif (SYS_CLK == 2)
    #define UART1_CONF_BAUD     BRR_DIV(115200, 48M)
#elif (SYS_CLK == 3)
    #define UART1_CONF_BAUD     BRR_DIV(115200, 64M)
#else
    #define UART1_CONF_BAUD     BRR_115200
#endif //SYS_CLK
#endif

#define UART1_FIFO_RXTL     8


/*
 * IMPORT MODULES
 ****************************************************************************************
 */

#undef  RBUF_SIZE
#define RBUF_SIZE           UART1_RBUF_SIZE
#include "rbuf.h"

/// RingBuffer for UART1
static rbuf_t uart1RbRx;


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void uart1Rb_Init(void)
{
    // uart init & conf
    uart_init(UART1_PORT, PA_UART1_TX, PA_UART1_RX);
    uart_conf(UART1_PORT, UART1_CONF_BAUD, UART1_CONF_LCRS);
    
    // enable uart IR
    uart_fctl(UART1_PORT, FCR_FIFOEN_BIT | FCR_RXTL_8BYTE, 
                20/*bits_rto*/, UART_IR_RXRD_BIT | UART_IR_RTO_BIT);

    // empty buffer
    rbuf_init(&uart1RbRx);
    
    NVIC_EnableIRQ(UART1_IRQn);
}

void uart1Rb_Reset(void)
{
    rbuf_init(&uart1RbRx);
}

uint16_t uart1Rb_Len(void)
{
    return rbuf_len(&uart1RbRx);
}

uint16_t uart1Rb_Read(uint8_t *buff, uint16_t max)
{
    return rbuf_gets(&uart1RbRx, buff, max);
}

void UART1_IRQHandler(void)
{
    uint32_t state = UART1->IFM.Word; // UART1->RIF.Word;
    
    if (state & 0x01) //(BIT_RXRD)
    {
        UART1->IDR.RXRD = 1; // Disable RXRD Interrupt
        
        for (uint8_t i = 0; i < UART1_FIFO_RXTL; i++)
        {
            rbuf_putc(&uart1RbRx, UART1->RBR);
        }
        
        UART1->ICR.RXRD = 1; // Clear RXRD Interrupt Flag
        UART1->IER.RXRD = 1; // Enable RXRD Interrupt
    }
    
    if (state & 0x10) //(BIT_RTO)
    {
        UART1->IDR.RTO = 1; // Disable RTO Interrupt
        
        while (UART1->SR.RFNE)
        {
            rbuf_putc(&uart1RbRx, UART1->RBR);
        }
        
        UART1->ICR.RTO = 1; // Clear RTO Interrupt Flag
        UART1->IER.RTO = 1; // Enable RTO Interrupt
    }
}

#endif  //(USE_UART1)
