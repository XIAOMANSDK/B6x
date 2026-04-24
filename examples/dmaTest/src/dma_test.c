/**
 ****************************************************************************************
 *
 * @file dma_test.c
 *
 * @brief DMA UART transfer demo.
 *
 * @details
 * This example demonstrates DMA usage in UART communication:
 * - RX channel: Ping-Pong mode with dual-buffer continuous receive
 * - TX channel: Basic mode, on-demand data transmission
 * - Supports both interrupt and polling modes
 *
 * Operation:
 * 1. DMA RX: Uses Ping-Pong mode to alternate between dual half-buffers
 * 2. UART RTO: Handles incomplete data packets via receive timeout interrupt
 * 3. DMA TX: Reads data from RX buffer and transmits
 * 4. Buffer management: Ring buffer for data storage and retrieval
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

#define DMA_IRQ_MODE       (1) ///< Mode select: 1=interrupt, 0=polling

/// DMA channel configuration
#define DMA_CH_UART_RX     DMA_CH0 ///< UART RX channel, Ping-Pong mode
#define DMA_CH_UART_TX     DMA_CH1 ///< UART TX channel, Basic mode

/// UART port and parameters
#define TEST_PORT          0      ///< 0:UART1_PORT  1:UART2_PORT
#define TEST_BAUD          BRR_DIV(115200, 16M)
#define TEST_LCRS          LCR_BITS(8, 1, none)

#define TEST_RTOR          (100)  ///< Receive timeout (n)SYM=(n/10)bytes
#define TEST_FCTL          (FCR_FIFOEN_BIT | FCR_RXTL_1BYTE)
#define TEST_INTR          (UART_IR_RTO_BIT)     ///< Enable receive timeout interrupt

/// Avoid conflict with debug UART pins
#define PA_UART_TX         (6)    ///< UART TX pin
#define PA_UART_RX         (7)    ///< UART RX pin

/// Status indicator GPIO pins
#define GPIO_RUN           GPIO08 ///< Running status
#define GPIO_TX_DONE       GPIO09 ///< TX complete
#define GPIO_RX_PING       GPIO10 ///< Ping buffer RX complete
#define GPIO_RX_PONG       GPIO11 ///< Pong buffer RX complete
#define GPIO_RX_RTOR       GPIO12 ///< Receive timeout indicator


/*
 * FUNCTIONS
 ****************************************************************************************
 */

/// *************** UART buffer management ***************
#define TXD_BUFF_SIZE      (0x10)   ///< TX buffer size
#define RXD_BUFF_SIZE      (0x200)  ///< RX buffer size
#define RXD_BUFF_HALF      (RXD_BUFF_SIZE / 2) ///< Ping-Pong half-buffer size

static volatile uint16_t rxdHead;          ///< RX buffer head pointer (write position)
static volatile uint16_t rxdTail;          ///< RX buffer tail pointer (read position)
static uint8_t rxdBuffer[RXD_BUFF_SIZE];   ///< RX data buffer
static uint8_t txdBuffer[TXD_BUFF_SIZE];   ///< TX data buffer

/**
 ****************************************************************************************
 * @brief Get readable data length in RX buffer.
 *
 * @return Readable data byte count
 ****************************************************************************************
 */
UNUSED static uint16_t uart_size(void)
{
    return ((rxdHead + RXD_BUFF_SIZE - rxdTail) % RXD_BUFF_SIZE);
}

/**
 ****************************************************************************************
 * @brief Read data from RX buffer.
 *
 * @param[out] buff Data output buffer
 * @param[in]  max  Max read length
 *
 * @return Actual read length
 *
 * @note Supports ring buffer wrap-around read.
 ****************************************************************************************
 */
static uint16_t uart_read(uint8_t *buff, uint16_t max)
{
    uint16_t head = rxdHead;
    uint16_t tail = rxdTail;
    uint16_t tlen, len;

    if ((max == 0) || (head == tail))
    {
        return 0; // Buffer empty
    }

    // Calculate readable length
    len = (head + RXD_BUFF_SIZE - tail) % RXD_BUFF_SIZE;
    if (len > max) len = max;

    // Handle ring buffer boundary
    if ((head > tail) || (tail + len <= RXD_BUFF_SIZE))
    {
        // Continuous data, direct copy
        memcpy(&buff[0], (const void *)&rxdBuffer[tail], len);
    }
    else
    {
        // Data wraps around, copy in two segments
        tlen = RXD_BUFF_SIZE - tail;

        memcpy(&buff[0], (const void *)&rxdBuffer[tail], tlen);        // Tail segment
        memcpy(&buff[tlen], (const void *)&rxdBuffer[0], len - tlen);  // Head segment
    }
    rxdTail = (tail + len) % RXD_BUFF_SIZE;

    return len;
}

/// *************** DMA event handlers ***************
static volatile bool rxChnlAlt  = false;   ///< RX channel active buffer flag
static volatile bool txChnlBusy = false;   ///< TX channel busy flag

/**
 ****************************************************************************************
 * @brief UART RX DMA completion handler.
 *
 * @details In Ping-Pong mode, when a half-buffer receive completes:
 *          reload DMA config, toggle buffer flag, update head pointer,
 *          and generate a pulse indicator.
 ****************************************************************************************
 */
static void dmaUartRxDone(void)
{
    // Reload DMA config, returns whether switched to alternate channel
    rxChnlAlt = dma_chnl_reload(DMA_CH_UART_RX);

    if (rxChnlAlt)
    {
        // Switched to Pong buffer
        rxdHead = RXD_BUFF_HALF;
        // Ping buffer complete pulse
        GPIO_DAT_SET(GPIO_RX_PING);
        GPIO_DAT_CLR(GPIO_RX_PING);
    }
    else
    {
        // Switched to Ping buffer
        rxdHead = 0;
        // Pong buffer complete pulse
        GPIO_DAT_SET(GPIO_RX_PONG);
        GPIO_DAT_CLR(GPIO_RX_PONG);
    }
}

/**
 ****************************************************************************************
 * @brief UART RX timeout handler.
 *
 * @details When UART RX data stream stalls beyond configured timeout:
 *          check RTO flag, clear interrupt, compute received data length
 *          from DMA state, and update head pointer.
 ****************************************************************************************
 */
static void dmaUartRxRtor(void)
{
    #if (TEST_PORT == 0)
    uint32_t iflag = UART1->IFM.Word;
    #else
    uint32_t iflag = UART2->IFM.Word;
    #endif

    if (iflag & UART_IR_RTO_BIT)
    {
        GPIO_DAT_SET(GPIO_RX_RTOR);

        #if (TEST_PORT == 0)
            UART1->ICR.Word = UART_IR_RTO_BIT;
        #else
            UART2->ICR.Word = UART_IR_RTO_BIT;
        #endif

        // Calculate received data position from current DMA channel
        if (rxChnlAlt)
        {
            // Currently on alternate channel (Pong)
            rxdHead = RXD_BUFF_HALF + (RXD_BUFF_HALF - dma_chnl_remain(DMA_CH_UART_RX | DMA_CH_ALT));
        }
        else
        {
            // Currently on primary channel (Ping)
            rxdHead = 0 + (RXD_BUFF_HALF - dma_chnl_remain(DMA_CH_UART_RX));
        }

        GPIO_DAT_CLR(GPIO_RX_RTOR);
    }
}

#if (DMA_IRQ_MODE)
/**
 ****************************************************************************************
 * @brief DMA interrupt service routine.
 *
 * @details Handles DMA channel completion interrupts:
 *          read IFR, disable triggered interrupts, clear flags,
 *          dispatch to channel handlers, re-enable interrupts.
 ****************************************************************************************
 */
void DMAC_IRQHandler(void)
{
    uint32_t iflag = DMACHCFG->IFLAG0;

    GPIO_DAT_SET(GPIO_RUN);

    // Disable triggered interrupts
    DMACHCFG->IEFR0 &= ~iflag;
    // Clear interrupt flags
    DMACHCFG->ICFR0 = iflag;

    // Handle RX channel interrupt
    if (iflag & (1UL << DMA_CH_UART_RX))
    {
        dmaUartRxDone();
    }

    // Handle TX channel interrupt
    if (iflag & (1UL << DMA_CH_UART_TX))
    {
        txChnlBusy = false;
        GPIO_DAT_SET(GPIO_TX_DONE);
        GPIO_DAT_CLR(GPIO_TX_DONE);
    }

    // Re-enable interrupts
    DMACHCFG->IEFR0 |= iflag;

    GPIO_DAT_CLR(GPIO_RUN);
}

/**
 ****************************************************************************************
 * @brief UART1 interrupt service routine.
 ****************************************************************************************
 */
void UART1_IRQHandler(void)
{
    dmaUartRxRtor();
}

/**
 ****************************************************************************************
 * @brief UART2 interrupt service routine.
 ****************************************************************************************
 */
void UART2_IRQHandler(void)
{
    dmaUartRxRtor();
}

/**
 ****************************************************************************************
 * @brief DMA UART main loop handler (interrupt mode).
 *
 * @details Checks TX channel status, reads from RX buffer, configures DMA TX.
 ****************************************************************************************
 */
static void dmaUartLoop(void)
{
    uint16_t len;

    if (txChnlBusy)
    {
        return;
    }

    len = uart_read(txdBuffer, TXD_BUFF_SIZE);
    if (len > 0)
    {
        txChnlBusy = true;

        #if (TEST_PORT == 0)
        DMA_UARTx_TX_CONF(DMA_CH_UART_TX, 1, txdBuffer, len, CCM_BASIC);
        #else
        DMA_UARTx_TX_CONF(DMA_CH_UART_TX, 2, txdBuffer, len, CCM_BASIC);
        #endif
    }
}

/**
 ****************************************************************************************
 * @brief Send data via DMA UART (interrupt mode).
 *
 * @param[in] data Data to send
 * @param[in] len  Data length
 ****************************************************************************************
 */
UNUSED static void dmaUartSend(const uint8_t *data, uint16_t len)
{
    txChnlBusy = true;

    #if (TEST_PORT == 0)
    DMA_UARTx_TX_CONF(DMA_CH_UART_TX, 1, data, len, CCM_BASIC);
    #else
    DMA_UARTx_TX_CONF(DMA_CH_UART_TX, 2, data, len, CCM_BASIC);
    #endif

    while (txChnlBusy) {}
}

#else  // Polling mode

/**
 ****************************************************************************************
 * @brief DMA polling handler.
 *
 * @details In polling mode: check RX channel completion, handle RX done or timeout.
 ****************************************************************************************
 */
static void dmaPolling(void)
{
    if (dma_chnl_done(DMA_CH_UART_RX))
    {
        GPIO_DAT_SET(GPIO_RUN);
        dmaUartRxDone();
        GPIO_DAT_CLR(GPIO_RUN);
    }
    else
    {
        dmaUartRxRtor();
    }
}

/**
 ****************************************************************************************
 * @brief DMA UART main loop handler (polling mode).
 *
 * @details Checks TX channel done status, reads from RX buffer, configures DMA TX.
 ****************************************************************************************
 */
static void dmaUartLoop(void)
{
    uint16_t len;

    if (txChnlBusy)
    {
        if (!dma_chnl_done(DMA_CH_UART_TX))
        {
            return;
        }

        txChnlBusy = false;
        GPIO_DAT_SET(GPIO_TX_DONE);
        GPIO_DAT_CLR(GPIO_TX_DONE);
    }

    len = uart_read(txdBuffer, TXD_BUFF_SIZE);
    if (len > 0)
    {
        txChnlBusy = true;

        #if (TEST_PORT == 0)
        DMA_UARTx_TX_CONF(DMA_CH_UART_TX, 1, txdBuffer, len, CCM_BASIC);
        #else
        DMA_UARTx_TX_CONF(DMA_CH_UART_TX, 2, txdBuffer, len, CCM_BASIC);
        #endif
    }
}

/**
 ****************************************************************************************
 * @brief Send data via DMA UART (polling mode).
 *
 * @param[in] data Data to send
 * @param[in] len  Data length
 ****************************************************************************************
 */
UNUSED static void dmaUartSend(const uint8_t *data, uint16_t len)
{
    #if (TEST_PORT == 0)
    DMA_UARTx_TX_CONF(DMA_CH_UART_TX, 1, data, len, CCM_BASIC);
    #else
    DMA_UARTx_TX_CONF(DMA_CH_UART_TX, 2, data, len, CCM_BASIC);
    #endif

    while (!dma_chnl_done(DMA_CH_UART_TX)) {}
}

#endif

/**
 ****************************************************************************************
 * @brief DMA test main function.
 *
 * @details Complete DMA UART test flow:
 *          1. GPIO init for status indicators
 *          2. DMA module init
 *          3. UART and DMA channel configuration
 *          4. Interrupt configuration (interrupt mode)
 *          5. Main loop processing
 ****************************************************************************************
 */
void dmaTest(void)
{
    // Configure GPIO as output for status indicators
    GPIO_DIR_SET_LO(GPIO_RUN | GPIO_TX_DONE | GPIO_RX_PING | GPIO_RX_PONG | GPIO_RX_RTOR);

    dma_init();

    #if (TEST_PORT == 0)
    // Init DMA channels
    DMA_UARTx_TX_INIT(DMA_CH_UART_TX, 1);
    DMA_UARTx_RX_INIT(DMA_CH_UART_RX, 1);

    // Configure RX channel in Ping-Pong mode
    DMA_UARTx_RX_CONF(DMA_CH_UART_RX, 1, &rxdBuffer[0], RXD_BUFF_HALF, CCM_PING_PONG);
    DMA_UARTx_RX_CONF(DMA_CH_UART_RX | DMA_CH_ALT, 1, &rxdBuffer[RXD_BUFF_HALF], RXD_BUFF_HALF, CCM_PING_PONG);
    #else
    // Init DMA channels
    DMA_UARTx_TX_INIT(DMA_CH_UART_TX, 2);
    DMA_UARTx_RX_INIT(DMA_CH_UART_RX, 2);

    // Configure RX channel in Ping-Pong mode
    DMA_UARTx_RX_CONF(DMA_CH_UART_RX, 2, &rxdBuffer[0], RXD_BUFF_HALF, CCM_PING_PONG);
    DMA_UARTx_RX_CONF(DMA_CH_UART_RX | DMA_CH_ALT, 2, &rxdBuffer[RXD_BUFF_HALF], RXD_BUFF_HALF, CCM_PING_PONG);
    #endif

    #if !((DBG_MODE == 1) && (TEST_PORT == 0))
    // Init UART parameters
    uart_init(TEST_PORT, PA_UART_TX, PA_UART_RX);
    uart_conf(TEST_PORT, TEST_BAUD, TEST_LCRS);
    #endif

    uart_fctl(TEST_PORT, TEST_FCTL, TEST_RTOR, TEST_INTR);
    uart_mctl(TEST_PORT, 1);  // Enable UART DMA mode

    #if (DMA_IRQ_MODE)
    // Enable DMA interrupts
    DMACHCFG->IEFR0 = (1UL << DMA_CH_UART_RX) | (1UL << DMA_CH_UART_TX);
    NVIC_EnableIRQ(DMAC_IRQn);

    #if (TEST_PORT == 0)
    NVIC_EnableIRQ(UART1_IRQn);
    #else
    NVIC_EnableIRQ(UART2_IRQn);
    #endif

    __enable_irq();
    #endif

    while (1)
    {
        #if !(DMA_IRQ_MODE)
        dmaPolling();
        #endif
        dmaUartLoop();
    }
}
