/**
 ****************************************************************************************
 *
 * @file uart2Rb.c
 *
 * @brief Demo of UART2 Interrupt-Mode with RingBuffer. *User should override it*
 *
 ****************************************************************************************
 */

#include <stdint.h>
#include "b6x.h"
#include "uartRb.h"

#if (USE_UART2)  // Remove UARTx_IRQHandler if unused

/*
 * DEFINES
 ****************************************************************************************
 */

#if !defined(UART2_RBUF_SIZE)
    #define UART2_RBUF_SIZE     0x100
#endif

#if !defined(UART2_CONF_LCRS)
    #define UART2_CONF_LCRS     LCR_BITS(8, 1, none) //default
#endif

#if !defined(UART2_CONF_BAUD)
#if (SYS_CLK == 1)
    #define UART2_CONF_BAUD     BRR_DIV(115200, 32M)
#elif (SYS_CLK == 2)
    #define UART2_CONF_BAUD     BRR_DIV(115200, 48M)
#elif (SYS_CLK == 3)
    #define UART2_CONF_BAUD     BRR_DIV(115200, 64M)
#else
    #define UART2_CONF_BAUD     BRR_115200
#endif //SYS_CLK
#endif

#define UART2_FIFO_RXTL     8


/*
 * IMPORT MODULES
 ****************************************************************************************
 */

#undef  RBUF_SIZE
#define RBUF_SIZE           UART2_RBUF_SIZE
#include "rbuf.h"

/// RingBuffer for UART2
static rbuf_t uart2RbRx;


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void uart2Rb_Init(void)
{
    // uart init & conf
    uart_init(UART2_PORT, PA_UART2_TX, PA_UART2_RX);
    uart_conf(UART2_PORT, UART2_CONF_BAUD, UART2_CONF_LCRS);
    
    // enable uart IR
    uart_fctl(UART2_PORT, FCR_FIFOEN_BIT | FCR_RXTL_8BYTE, 
                20/*bits_rto*/, UART_IR_RXRD_BIT | UART_IR_RTO_BIT);

    // empty buffer
    rbuf_init(&uart2RbRx);
    
    NVIC_EnableIRQ(UART2_IRQn);
}

void uart2Rb_Reset(void)
{
    rbuf_init(&uart2RbRx);
}

uint16_t uart2Rb_Len(void)
{
    return rbuf_len(&uart2RbRx);
}

uint16_t uart2Rb_Read(uint8_t *buff, uint16_t max)
{
    return rbuf_gets(&uart2RbRx, buff, max);
}

void UART2_IRQHandler(void)
{
    uint32_t state = UART2->IFM.Word; // UART2->RIF.Word;
    
    if (state & 0x01) //(BIT_RXRD)
    {
        UART2->IDR.RXRD = 1; // Disable RXRD Interrupt
        
        for (uint8_t i = 0; i < UART2_FIFO_RXTL; i++)
        {
            rbuf_putc(&uart2RbRx, UART2->RBR);
        }
        
        UART2->ICR.RXRD = 1; // Clear RXRD Interrupt Flag
        UART2->IER.RXRD = 1; // Enable RXRD Interrupt
    }
    
    if (state & 0x10) //(BIT_RTO)
    {
        UART2->IDR.RTO = 1; // Disable RTO Interrupt
        
        while (UART2->SR.RFNE)
        {
            rbuf_putc(&uart2RbRx, UART2->RBR);
        }
        
        UART2->ICR.RTO = 1; // Clear RTO Interrupt Flag
        UART2->IER.RTO = 1; // Enable RTO Interrupt
    }
}

#endif  //(USE_UART2)
