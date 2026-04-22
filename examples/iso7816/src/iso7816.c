/**
 ****************************************************************************************
 *
 * @file iso7816.c
 *
 * @brief ISO7816 smart card interface driver
 *
 * @details
 * Implements UART-based ISO7816 smart card communication with:
 * - DMA ping-pong mode for efficient data reception
 * - Ring buffer for received data
 * - Blocking transmit (runs from SRAM)
 *
 ****************************************************************************************
 */

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

/// Get UARTx pointer, port: 0=UART1, 1=UART2
#define UART_PTR(port)         ((UART_TypeDef *)(UART1_BASE + (port) * 0x1000))

/// UART port used for ISO7816
#define ISO7816_PORT                UART2_PORT

/*
 * CONFIG
 ****************************************************************************************
 */

#define CFG_7816_DMA        1

#if (CFG_7816_DMA)
#include "dma.h"
#define ISO_7816_DMA_CHAN   DMA_CH6
#define RBUF_HALF_SIZE      (RBUF_SIZE / 2)
static volatile bool pong;
#endif

/// Ring buffer for ISO7816 received data
static rbuf_t iso7816RbRx;

/*
 * LOCAL FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialize UART in smart card (7816) mode
 *
 * @param[in] port     UART port index (0=UART1, 1=UART2)
 * @param[in] io_clk   Clock pin
 * @param[in] io_data  Data pin (bidirectional)
 ****************************************************************************************
 */
static void uart_7816_init(uint8_t port, uint8_t io_clk, uint8_t io_data)
{
    csc_output(io_clk, CSC_UART1_SCK + port);

    csc_output(io_data, CSC_UART1_TXD + port * 2);
    csc_input(io_data,  CSC_UART1_TXD + port * 2);

    // CLK, TXD: CSC + pull-up + level-1 drive
    iom_ctrl(io_clk,  IOM_SEL_CSC | IOM_PULLUP | IOM_DRV_LVL1);
    iom_ctrl(io_data, IOM_SEL_CSC | IOM_PULLUP | IOM_DRV_LVL1 | IOM_INPUT | IOM_OPENDRAIN);

    RCC_APBCLK_EN(1 << (RCC_UART1_CLKEN_RUN_POS + port));
    RCC_APBRST_REQ(1 << (RCC_UART1_RSTREQ_POS + port));
}

/**
 ****************************************************************************************
 * @brief Configure UART for smart card communication
 *
 * @param[in] port     UART port index
 * @param[in] cfg_BRR  Baud rate register value
 * @param[in] cfg_LCR  Line control register value
 ****************************************************************************************
 */
static void uart_7816_conf(uint8_t port, uint16_t cfg_BRR, uint16_t cfg_LCR)
{
    UART_TypeDef *uart = UART_PTR(port);

    uart->CR.GT  = 0x00;
    // Smart card clock = p_clk / (2 * (psc + 1))
    uart->CR.PSC = ISO7816_CLK_4M;

    uart->MCR.SCCNT  = 0x03;
    uart->MCR.SCNACK = 1; // Parity error: NACK retransmit

    // Clear all
    uart->LCR.Word   = 0;

    // Update baud rate
    uart->LCR.BRWEN  = 1;
    uart->BRR        = cfg_BRR;
    uart->LCR.BRWEN  = 0;

    // Enable FIFO mode, reset
    uart->FCR.Word |= 0x07; // FIFOEN RFRST TFRST

    // Config params, enable RX
    uart->LCR.Word = cfg_LCR | LCR_RXEN_BIT;

    uart->MCR.CLKEN = 1; // Smart card clock enable
    uart->MCR.SCEN  = 1; // Smart card mode enable
}

/*
 * PUBLIC FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialize ISO7816 UART, DMA, and interrupts
 ****************************************************************************************
 */
void iso7816Init(void)
{
    rbuf_init(&iso7816RbRx);

    uart_7816_init(ISO7816_PORT, PA_ISO7816_CLK, PA_ISO7816_DATA);
    uart_7816_conf(ISO7816_PORT, ISO7816_BAUD, ISO7816_LCRS);

    #if (CFG_7816_DMA)
        // DMA + RTO mode
        uart_fctl(ISO7816_PORT, FCR_FIFOEN_BIT | FCR_RXTL_1BYTE,
                    40 /* bits_rto */, UART_IR_RTO_BIT);
        uart_mctl(ISO7816_PORT, 1);    // DMA enable

        pong = false;

        dma_init();
        DMA_UARTx_RX_INIT(ISO_7816_DMA_CHAN, 2);
        DMA_UARTx_RX_CONF(ISO_7816_DMA_CHAN, 2, iso7816RbRx.data, RBUF_HALF_SIZE, CCM_PING_PONG);
        DMA_UARTx_RX_CONF(ISO_7816_DMA_CHAN | DMA_CH_ALT, 2, (iso7816RbRx.data + RBUF_HALF_SIZE), RBUF_HALF_SIZE, CCM_PING_PONG);
        dma_chnl_ctrl(ISO_7816_DMA_CHAN, CHNL_EN);

        DMACHNL_INT_EN(ISO_7816_DMA_CHAN);
        NVIC_EnableIRQ(DMAC_IRQn);
    #else
        // Interrupt mode
        uart_fctl(ISO7816_PORT, FCR_FIFOEN_BIT | FCR_RXTL_8BYTE,
                    20 /* bits_rto */, UART_IR_RXRD_BIT | UART_IR_RTO_BIT);
    #endif

    NVIC_EnableIRQ(UART2_IRQn);

    GPIO_DIR_SET_LO(BIT(PA_ISO7816_RST));
    GPIO_DAT_SET(BIT(PA_ISO7816_RST));
}

/**
 ****************************************************************************************
 * @brief Send data via ISO7816 (blocking, runs from SRAM)
 *
 * @param[in] len   Data length
 * @param[in] data  Data to send
 *
 * @note Disables RX during transmission, re-enables after bus idle
 ****************************************************************************************
 */
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

    while (!(uart->SR.TBEM)); // Wait TX finish
    while (uart->SR.BUSY);    // Wait idle state
    uart->LCR.RXEN = 1;       // Re-enable RX

    GLOBAL_INT_RESTORE();
}

/**
 ****************************************************************************************
 * @brief Reset the receive ring buffer
 ****************************************************************************************
 */
void iso7816RbReset(void)
{
    rbuf_init(&iso7816RbRx);
}

/**
 ****************************************************************************************
 * @brief Get number of bytes in receive ring buffer
 *
 * @return Number of bytes available
 ****************************************************************************************
 */
uint16_t iso7816RbLen(void)
{
    return rbuf_len(&iso7816RbRx);
}

/**
 ****************************************************************************************
 * @brief Read data from receive ring buffer
 *
 * @param[out] buff  Destination buffer
 * @param[in]  max   Maximum bytes to read
 *
 * @return Number of bytes actually read
 ****************************************************************************************
 */
uint16_t iso7816RbRead(uint8_t *buff, uint16_t max)
{
    return rbuf_gets(&iso7816RbRx, buff, max);
}

/*
 * INTERRUPT HANDLERS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief UART2 interrupt handler for ISO7816
 ****************************************************************************************
 */
void UART2_IRQHandler(void)
{
    GPIO_DIR_SET_HI(GPIO05);

    uint32_t irq_state = UART2->IFM.Word;

    #if (CFG_7816_DMA)
        if (irq_state & UART_IR_RTO_BIT)
        {
            GPIO_DIR_SET_HI(GPIO04);
            UART2->ICR.Word = UART_IR_RTO_BIT;

            uint16_t remain_len;
            bool alter = dma_chnl_remain_pingpong(ISO_7816_DMA_CHAN, &remain_len);

            // Update head based on ping-pong position
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
        if (irq_state & BIT(UART_INT_RXS_POS))
        {
            UART2->IDR.RXS = 1;
            while (UART2->SR.RFNE)
            {
                rbuf_putc(&iso7816RbRx, UART2->RBR);
            }
            UART2->ICR.RXS = 1;
            UART2->IER.RXS = 1;
        }
        else if (irq_state & BIT(UART_INT_RXRD_POS))
        {
            UART2->IDR.RXRD = 1;
            while (UART2->SR.RFNE)
            {
                rbuf_putc(&iso7816RbRx, UART2->RBR);
            }
            UART2->ICR.RXRD = 1;
            UART2->IER.RXRD = 1;
        }
        else if (irq_state & BIT(UART_INT_RTO_POS))
        {
            GPIO_DIR_SET_HI(GPIO04);
            UART2->IDR.RTO = 1;
            while (UART2->SR.RFNE)
            {
                rbuf_putc(&iso7816RbRx, UART2->RBR);
            }
            UART2->ICR.RTO = 1;
            UART2->IER.RTO = 1;
        }
        else
        {
            DEBUG("irq_state[0x%X]", irq_state);
            UART2->ICR.TXS = 1;
        }
    #endif

    GPIO_DIR_SET_LO(GPIO04);
    GPIO_DIR_SET_LO(GPIO05);
}

#if (CFG_7816_DMA)
/**
 ****************************************************************************************
 * @brief DMA RX completion handler for ping-pong buffer
 ****************************************************************************************
 */
__STATIC_INLINE void uart_dma_rx_done(void)
{
    if (dma_chnl_reload(ISO_7816_DMA_CHAN))
    {
        iso7816RbRx.head = RBUF_HALF_SIZE; // Head to Pong
        pong = true;
    }
    else
    {
        iso7816RbRx.head = 0; // Head to Ping
        pong = false;
    }
}

/**
 ****************************************************************************************
 * @brief DMA channel interrupt handler
 ****************************************************************************************
 */
void DMAC_IRQHandler(void)
{
    uint32_t iflag = DMACHNL_INT_GET(ISO_7816_DMA_CHAN);

    if (iflag)
    {
        DMACHNL_INT_DIS(ISO_7816_DMA_CHAN);
        DMACHNL_INT_CLR(ISO_7816_DMA_CHAN);

        uart_dma_rx_done();

        DMACHNL_INT_EN(ISO_7816_DMA_CHAN);
    }
}
#endif
