#include "drvs.h"
#include "iso7816.h"
#include "rbuf.h"

#if (DBG_ISO7816)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, (int)__LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif

/// Get UARTx Pointer, 'port' : 0-UART1, 1-UART2
#define UART_PTR(port)         ((UART_TypeDef *)(UART1_BASE + (port) * 0x1000))
/// Uart Port and Params
#define ISO7816_PORT                UART2_PORT
/*
 * FUNCTIONS
 ****************************************************************************************
 */
#define CFG_7816_DMA        1

#if (CFG_7816_DMA)
#include "dma.h"
#define ISO_7816_DMA_CHAN   DMA_CH6
#define RBUF_HALF_SIZE      (RBUF_SIZE/2)
volatile bool pong;
#endif

/// RingBuffer for ISO7816
static rbuf_t iso7816RbRx;

// Smart Card
void uart_7816_init(uint8_t port, uint8_t io_clk, uint8_t io_data)
{
    //iocsc_uart(port, io_tx, io_rx);
    csc_output(io_clk, CSC_UART1_SCK + port);

    csc_output(io_data, CSC_UART1_TXD + port * 2);
    csc_input(io_data,  CSC_UART1_TXD + port * 2);

    //clk, txd  csc& pull-up & LVL1
    iom_ctrl(io_clk,  IOM_SEL_CSC | IOM_PULLUP | IOM_DRV_LVL1);
    iom_ctrl(io_data, IOM_SEL_CSC | IOM_PULLUP | IOM_DRV_LVL1 | IOM_INPUT | IOM_OPENDRAIN);

    // uart_clk_en rst_req
    RCC_APBCLK_EN(1 << (RCC_UART1_CLKEN_RUN_POS + port));
    RCC_APBRST_REQ(1 << (RCC_UART1_RSTREQ_POS + port));
}

void uart_7816_conf(uint8_t port, uint16_t cfg_BRR, uint16_t cfg_LCR)
{
    UART_TypeDef* uart = UART_PTR(port);

    uart->CR.GT  = 0x00;//guard time 12
    // smartcard clk = p_clk / (2 * (psc + 1))
    uart->CR.PSC = ISO7816_CLK_4M;

    uart->MCR.SCCNT  = 0x03;
    uart->MCR.SCNACK = 1;//parity error, NACK transform

    // clear all
    uart->LCR.Word   = 0;

    // update BaudRate
    uart->LCR.BRWEN  = 1;
    uart->BRR        = cfg_BRR; // (rcc_sysclk_get() + (baud >> 1)) / baud
    uart->LCR.BRWEN  = 0;

    // enable fifo mode, reset
    uart->FCR.Word |= 0x07; // FIFOEN RFRST TFRST

    // config params, enable
    uart->LCR.Word = cfg_LCR | LCR_RXEN_BIT;

//    uart->IER.RXS   = 1; // receive data interrupt
//    uart->IER.RXRD  = 1;

    uart->MCR.CLKEN = 1; //smartcard clock enable
    uart->MCR.SCEN  = 1; //smartcard mode enable
}

void iso7816Init(void)
{
    // empty buffer
    rbuf_init(&iso7816RbRx);

    uart_7816_init(ISO7816_PORT, PA_ISO7816_CLK, PA_ISO7816_DATA);
    uart_7816_conf(ISO7816_PORT, ISO7816_BAUD, ISO7816_LCRS);

    #if (CFG_7816_DMA)
        // DMA +RTO
        // enable uart IR
        uart_fctl(ISO7816_PORT, FCR_FIFOEN_BIT | FCR_RXTL_1BYTE,
                    40/*bits_rto*/,  UART_IR_RTO_BIT);  // 只开 RTO中断
        uart_mctl(ISO7816_PORT, 1);    // ESAM_PORT DMA使能

        pong = false;

        dma_init();
        DMA_UARTx_RX_INIT(ISO_7816_DMA_CHAN, 2);
        DMA_UARTx_RX_CONF(ISO_7816_DMA_CHAN, 2, iso7816RbRx.data, RBUF_HALF_SIZE, CCM_PING_PONG);
        DMA_UARTx_RX_CONF(ISO_7816_DMA_CHAN | DMA_CH_ALT, 2, (iso7816RbRx.data + RBUF_HALF_SIZE), RBUF_HALF_SIZE, CCM_PING_PONG);
        dma_chnl_ctrl(ISO_7816_DMA_CHAN, CHNL_EN);

        // enable UART1 DMA Channel Interrupt
        DMACHNL_INT_EN(ISO_7816_DMA_CHAN);
        NVIC_EnableIRQ(DMAC_IRQn);
    #else
        // enable uart IR
        uart_fctl(ISO7816_PORT, FCR_FIFOEN_BIT | FCR_RXTL_8BYTE,
                    20/*bits_rto*/, UART_IR_RXRD_BIT | UART_IR_RTO_BIT);
    #endif


    NVIC_EnableIRQ(UART2_IRQn);

    GPIO_DIR_SET_LO(BIT(PA_ISO7816_RST));
    GPIO_DAT_SET(BIT(PA_ISO7816_RST));
}

// 放在Sram中执行
__SRAMFN
void iso7816Send(uint16_t len, const uint8_t *data)
{
    GLOBAL_INT_DISABLE();
    UART_TypeDef *uart = UART_PTR(ISO7816_PORT);
    uart->LCR.RXEN = 0;
    while (len--)
    {
        while (!(uart->SR.TBEM));
        uart->TBR = *data++;
    }

    while (!(uart->SR.TBEM)); // wait tx finish
    while (uart->SR.BUSY);    // wait idle state
    uart->LCR.RXEN = 1;       // 软件开启接收使能
    GLOBAL_INT_RESTORE();     // 需要屏蔽其他中断干扰>100us  --20250409
}

void iso7816RbReset(void)
{
    rbuf_init(&iso7816RbRx);
}

uint16_t iso7816RbLen(void)
{
    return rbuf_len(&iso7816RbRx);
}

uint16_t iso7816RbRead(uint8_t *buff, uint16_t max)
{
    return rbuf_gets(&iso7816RbRx, buff, max);
}

void UART2_IRQHandler(void)
{
    GPIO_DIR_SET_HI(GPIO05);

    uint32_t irq_state = UART2->IFM.Word; // UART1->RIF.Word;

    #if (CFG_7816_DMA)
        if (irq_state & UART_IR_RTO_BIT)
        {
            GPIO_DIR_SET_HI(GPIO04);
            // clear rto
            UART2->ICR.Word = UART_IR_RTO_BIT;
            uint16_t remain_len;
            bool alter = dma_chnl_remain_pingpong(ISO_7816_DMA_CHAN, &remain_len);

            // update head to middle
            if (pong == alter)
            {
                if (alter)
                {
                    iso7816RbRx.head = RBUF_HALF_SIZE + (RBUF_HALF_SIZE - remain_len);
                }
                else
                {
                    iso7816RbRx.head = 0 + (RBUF_HALF_SIZE - remain_len);
                }
            }
        }
    #else
        if (irq_state & BIT(UART_INT_RXS_POS)) //(BIT_RXRD)
        {
            UART2->IDR.RXS = 1; // Disable RXS Interrupt

            while (UART2->SR.RFNE)
            {
                rbuf_putc(&iso7816RbRx, UART2->RBR);
            }

            UART2->ICR.RXS = 1; // Clear RXS Interrupt Flag
            UART2->IER.RXS = 1; // Enable RXS Interrupt
        }

        else if (irq_state & BIT(UART_INT_RXRD_POS))
        {
            UART2->IDR.RXRD = 1; // Disable RXRD Interrupt

            while (UART2->SR.RFNE)
            {
                rbuf_putc(&iso7816RbRx, UART2->RBR);
            }

            UART2->ICR.RXRD = 1; // Clear RXRD Interrupt Flag
            UART2->IER.RXRD = 1; // Enable RXRD Interrupt
        }

        else if (irq_state & BIT(UART_INT_RTO_POS))
        {
            GPIO_DIR_SET_HI(GPIO04);
            UART2->IDR.RTO = 1; // Disable RTO Interrupt

            while (UART2->SR.RFNE)
            {
                rbuf_putc(&iso7816RbRx, UART2->RBR);
            }

            UART2->ICR.RTO = 1; // Clear RTO Interrupt Flag
            UART2->IER.RTO = 1; // Enable RTO Interrupt
        }
        else
        {
            DEBUG("irq_state[0x%X]", irq_state);
            UART2->ICR.TXS = 1; // Clear RTO Interrupt Flag
        }
    #endif

    GPIO_DIR_SET_LO(GPIO04);
    GPIO_DIR_SET_LO(GPIO05);
}

#if (CFG_7816_DMA)
__STATIC_INLINE void uart1_dma_rx_done(void)
{
    if (dma_chnl_reload(ISO_7816_DMA_CHAN))
    {
        // head to Pong
        iso7816RbRx.head = RBUF_HALF_SIZE;
        pong = true;
    }
    else
    {
        // head to Ping
        iso7816RbRx.head = 0;
        pong = false;
    }
}

void DMAC_IRQHandler(void)
{
    uint32_t iflag = DMACHNL_INT_GET(ISO_7816_DMA_CHAN);

    if (iflag)
    {
        // disable intr
        DMACHNL_INT_DIS(ISO_7816_DMA_CHAN);

        // clear intr flag
        DMACHNL_INT_CLR(ISO_7816_DMA_CHAN);

        uart1_dma_rx_done();

        // re-enable intr
        DMACHNL_INT_EN(ISO_7816_DMA_CHAN);
    }
}
#endif
