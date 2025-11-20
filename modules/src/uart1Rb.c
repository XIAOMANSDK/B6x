/**
 ****************************************************************************************
 *
 * @file uart1Rb.c
 *
 * @brief Demo of UART1 Interrupt-Mode/DMA-Mode with RingBuffer. *User should override it*
 *
 * @note When the stop bit time is about 1.5 times the set time, CTMR needs to be used instead of RTO.
 ****************************************************************************************
 */

#include "drvs.h"
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

#ifndef CFG_UART_DMA
#define CFG_UART_DMA        1
#endif

#if (CFG_UART_DMA)
#define CFG_UART_CTMR       0  //When the stop time is about 1.5 times the set value, CTMR needs to be used

#include "dma.h"
#define UART1_DMA_CHAN      DMA_CH7
#define RBUF_SIZE           (0x300)
#define RBUF_HALF_SIZE      (RBUF_SIZE/2)

volatile bool pong;
#else
#define RBUF_SIZE           UART1_RBUF_SIZE
#endif

/*
 * IMPORT MODULES
 ****************************************************************************************
 */

#include "rbuf.h"

/// RingBuffer for UART1
static rbuf_t uart1RbRx;
/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */
#if (CFG_UART_CTMR)
#include "drvs.h"
#include "regs.h"

void uartTm_Init(void)
{
    RCC_APBCLK_EN(APB_CTMR_BIT);
    RCC_APBRST_REQ(APB_CTMR_BIT);

    CTMR->CR1.Word = 0; //.CEN = 0;
    
    csc_input(PA_UART1_RX, CSC_CTMR_CH1);
    
    // do not need filter
    CTMR->CCMR1.INPUT.IC1F = 0;
    // sel input capture src
    CTMR->CCMR1.INPUT.CC1S = 1;
    // only detect posedge
    CTMR->CCER.CC1P = 0;
    CTMR->CCER.CC1NP = 0;
    
    CTMR->CR1.URS = 1;
    CTMR->ARR = 50;           // Number of timeout bits
    CTMR->PSC = UART1->BRR;   // Baud rate value UART1->BRR
    
    CTMR->SMCR.SMS = 0x04; // reset mode
    CTMR->SMCR.TS = 0x05; // sel ti1 as input src
    
    CTMR->CCR1 = 0;
    CTMR->IER.Word = 1;
    CTMR->ICR.Word = 1;
    
    NVIC_EnableIRQ(CTMR_IRQn);
}

void CTMR_IRQHandler(void)
{
    GPIO_DIR_SET_HI(BIT(PA_CTMR_IRQH));
    GPIO_DIR_SET_LO(BIT(PA_CTMR_IRQH));
    
    // Disable UI Interrupt
    CTMR->IDR.Word = TMR_IR_UI_BIT; 
    
    CTMR->CR1.CEN = 0; // Turn off CTMR enable
    
    if (CTMR->RIF.Word & TMR_IR_UI_BIT)
    {
        // Clear Interrupt Flag
        CTMR->ICR.Word = TMR_IR_UI_BIT; 
        
        uint16_t remain_len;
        bool alter = dma_chnl_remain_pingpong(UART1_DMA_CHAN, &remain_len);

        // update head to middle
        if (pong == alter)
        {
            if (alter)
            {
                uart1RbRx.head = RBUF_HALF_SIZE + (RBUF_HALF_SIZE - remain_len);
            }
            else
            {
                uart1RbRx.head = 0 + (RBUF_HALF_SIZE - remain_len);
            }
        }
        
        UART1->ICR.EOB = 1; // Clear EOB Interrupt Flag
        UART1->IER.EOB = 1; // Enable RXRD Interrupt
    }
    
    // Enable UI Interrupt
    CTMR->IER.Word = TMR_IR_UI_BIT;
}
#endif

void uart1Rb_Init(void)
{
    // empty buffer
    rbuf_init(&uart1RbRx);
    
    // uart init & conf
    uart_init(UART1_PORT, PA_UART1_TX, PA_UART1_RX);
    uart_conf(UART1_PORT, UART1_CONF_BAUD, UART1_CONF_LCRS);
    
    #if (CFG_UART_DMA)
        #if (CFG_UART_CTMR)
        // Using EOB interrupt, the RXRD flag will be automatically cleared by DMA
        uart_fctl(UART1_PORT, (FCR_FIFOEN_BIT | FCR_RXTL_1BYTE), 0, /*UART_IR_RXRD_BIT | UART_IR_RXS_BIT |*/ UART_IR_EOB_BIT);
        uartTm_Init();
        #else
        uart_fctl(UART1_PORT, (FCR_FIFOEN_BIT | FCR_RXTL_1BYTE), 40, UART_IR_RTO_BIT);
        #endif

    uart_mctl(UART1_PORT, 1);
    
    pong = false;
    
    dma_init();
    DMA_UARTx_RX_INIT(UART1_DMA_CHAN, 1);
    DMA_UARTx_RX_CONF(UART1_DMA_CHAN, 1, uart1RbRx.data, RBUF_HALF_SIZE, CCM_PING_PONG);
    DMA_UARTx_RX_CONF(UART1_DMA_CHAN | DMA_CH_ALT, 1, (uart1RbRx.data + RBUF_HALF_SIZE), RBUF_HALF_SIZE, CCM_PING_PONG);    
    dma_chnl_ctrl(UART1_DMA_CHAN, CHNL_EN);
    
    // enable UART1 DMA Channel Interrupt
    DMACHNL_INT_EN(UART1_DMA_CHAN);
    NVIC_EnableIRQ(DMAC_IRQn);
    #else
    // enable uart IR
    uart_fctl(UART1_PORT, FCR_FIFOEN_BIT | FCR_RXTL_8BYTE, 
                20/*bits_rto*/, UART_IR_RXRD_BIT | UART_IR_RTO_BIT);
    #endif

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
    
    #if (CFG_UART_DMA)
        #if (CFG_UART_CTMR)
        if (state & 0x400) //(EOB_BIT)
        {
            UART1->IDR.EOB = 1; // Disable EOB Interrupt
            
            CTMR->CR1.CEN = 1; // Enable CTMR timeout count
            
            UART1->ICR.EOB = 1; // Clear EOB Interrupt Flag
        }
        #else
        if (state & UART_IR_RTO_BIT)
        {
            // clear rto
            UART1->ICR.Word = UART_IR_RTO_BIT;
            uint16_t remain_len;
            bool alter = dma_chnl_remain_pingpong(UART1_DMA_CHAN, &remain_len);

            // update head to middle
            if (pong == alter)
            {
                if (alter)
                {
                    uart1RbRx.head = RBUF_HALF_SIZE + (RBUF_HALF_SIZE - remain_len);
                }
                else
                {
                    uart1RbRx.head = 0 + (RBUF_HALF_SIZE - remain_len);
                }
            }
        }    
        #endif
    #else
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
    #endif
}

#if (CFG_UART_DMA)
__STATIC_INLINE void uart1_dma_rx_done(void)
{
    if (dma_chnl_reload(UART1_DMA_CHAN))
    {
        // head to Pong
        uart1RbRx.head = RBUF_HALF_SIZE;
        pong = true;
    }
    else
    {
        // head to Ping
        uart1RbRx.head = 0;
        pong = false;
    }
}

void DMAC_IRQHandler(void)
{
    uint32_t iflag = DMACHNL_INT_GET(UART1_DMA_CHAN);

    if (iflag)
    {
        // disable intr
        DMACHNL_INT_DIS(UART1_DMA_CHAN);
        
        // clear intr flag
        DMACHNL_INT_CLR(UART1_DMA_CHAN);
        
        uart1_dma_rx_done();
        
        // re-enable intr
        DMACHNL_INT_EN(UART1_DMA_CHAN);
    }
}
#endif

#endif  //(USE_UART1)
