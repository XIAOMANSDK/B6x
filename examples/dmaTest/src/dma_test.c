/**
 ****************************************************************************************
 *
 * @file dma_test.c
 *
 * @brief DMA demo for UART Rx Tx.
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

#define DMA_IRQ_MODE       (1) // 1-Interrupt, 0-Polling

/// DMA chnl & ccm
#define DMA_CH_UART_RX     DMA_CH0 // Ping-Pong mode
#define DMA_CH_UART_TX     DMA_CH1 // Basic mode

/// Uart Port and Params
#define TEST_PORT          UART1_PORT
#define TEST_BAUD          BRR_DIV(115200, 16M)
#define TEST_LCRS          LCR_BITS(8, 1, none)

#define TEST_RTOR          (100) // Timeout (n)SYM=(n/10)Bytes
#define TEST_FCTL          (FCR_FIFOEN_BIT | FCR_RXTL_1BYTE)
#define TEST_INTR          (UART_IR_RTO_BIT)

/// Avoid conflict GPIOs of UART_DBG if diff port
#define PA_UART_TX         (6)
#define PA_UART_RX         (7)

/// GPIOs for watching
#define GPIO_RUN           GPIO08
#define GPIO_TX_DONE       GPIO09
#define GPIO_RX_PING       GPIO10
#define GPIO_RX_PONG       GPIO11
#define GPIO_RX_RTOR       GPIO12


/*
 * FUNCTIONS
 ****************************************************************************************
 */

/// *************** UART Buffer Func *************** 
#define TXD_BUFF_SIZE      (0x10)
#define RXD_BUFF_SIZE      (0x200)
#define RXD_BUFF_HALF      (RXD_BUFF_SIZE / 2) // Ping-Pong half

volatile uint16_t rxdHead;
volatile uint16_t rxdTail;
uint8_t rxdBuffer[RXD_BUFF_SIZE]; // __attribute__((aligned(4)))
uint8_t txdBuffer[TXD_BUFF_SIZE];

uint16_t uart_size(void)
{
    return ((rxdHead + RXD_BUFF_SIZE - rxdTail) % RXD_BUFF_SIZE);
}

uint16_t uart_read(uint8_t *buff, uint16_t max)
{
    uint16_t head = rxdHead;
    uint16_t tail = rxdTail;
    uint16_t tlen, len;
    if ((max == 0) || (head == tail))
    {
        return 0; // empty
    }

    len = (head + RXD_BUFF_SIZE - tail) % RXD_BUFF_SIZE;
    if (len > max) len = max;

    if ((head > tail) || (tail + len <= RXD_BUFF_SIZE))
    {
        memcpy(&buff[0], (const void *)&rxdBuffer[tail], len);
    }
    else
    {
        tlen = RXD_BUFF_SIZE - tail;

        memcpy(&buff[0], (const void *)&rxdBuffer[tail], tlen); // tail_len
        memcpy(&buff[tlen], (const void *)&rxdBuffer[0], len - tlen); // head_len
    }
    rxdTail = (tail + len) % RXD_BUFF_SIZE;

    return len; // count
}

/// *************** DMA Event Func *************** 
volatile bool rxChnlAlt  = false;
volatile bool txChnlBusy = false;

static void dmaUartRxDone(void)
{
    rxChnlAlt = dma_chnl_reload(DMA_CH_UART_RX);
    if (rxChnlAlt)
    {
        // head to Pong
        rxdHead = RXD_BUFF_HALF;
        // Ping done pulse
        GPIO_DAT_SET(GPIO_RX_PING);
        GPIO_DAT_CLR(GPIO_RX_PING);
    }
    else
    {
        // head to Ping
        rxdHead = 0;
        // Pong done pulse
        GPIO_DAT_SET(GPIO_RX_PONG);
        GPIO_DAT_CLR(GPIO_RX_PONG);
    }
}

static void dmaUartRxRtor(void)
{
    uint32_t iflag = UART1->IFM.Word; // UART1->RIF.Word;

    if (iflag & UART_IR_RTO_BIT)
    {
        GPIO_DAT_SET(GPIO_RX_RTOR);
        
        // clear rto
        UART1->ICR.Word = UART_IR_RTO_BIT;
        
        // update head to middle
        if (rxChnlAlt)
        {
            rxdHead = RXD_BUFF_HALF + (RXD_BUFF_HALF - dma_chnl_remain(DMA_CH_UART_RX | DMA_CH_ALT));
        }
        else
        {
            rxdHead = 0 + (RXD_BUFF_HALF - dma_chnl_remain(DMA_CH_UART_RX));
        }
        
        GPIO_DAT_CLR(GPIO_RX_RTOR);
    }
}

#if (DMA_IRQ_MODE)
void DMAC_IRQHandler(void)
{
    uint32_t iflag = DMACHCFG->IFLAG0;
    
    GPIO_DAT_SET(GPIO_RUN);
    
    // disable intr
    DMACHCFG->IEFR0 &= ~iflag;
    // clear intr flag
    DMACHCFG->ICFR0 = iflag;
    
    if (iflag & (1UL << DMA_CH_UART_RX))
    {
        dmaUartRxDone();
    }
    
    if (iflag & (1UL << DMA_CH_UART_TX))
    {
        txChnlBusy = false;
        GPIO_DAT_SET(GPIO_TX_DONE);
        GPIO_DAT_CLR(GPIO_TX_DONE);
    }
    
    // re-enable intr
    DMACHCFG->IEFR0 |= iflag;
    
    GPIO_DAT_CLR(GPIO_RUN);
}

void UART1_IRQHandler(void)
{
    dmaUartRxRtor();
}

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
        DMA_UARTx_TX_CONF(DMA_CH_UART_TX, 1, txdBuffer, len, CCM_BASIC);
    }
}

void dmaUartSend(const uint8_t *data, uint16_t len)
{
    txChnlBusy = true;
    DMA_UARTx_TX_CONF(DMA_CH_UART_TX, 1, data, len, CCM_BASIC);

    while (txChnlBusy); // wait done
    //while (!(UART1->SR.TEM)); //Wait Transmitter Empty
}
#else
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

static void dmaUartLoop(void)
{
    uint16_t len;
    
    if (txChnlBusy)
    {
        if (!dma_chnl_done(DMA_CH_UART_TX))
        {
            return; // wait done
        }
        
        txChnlBusy = false;
        GPIO_DAT_SET(GPIO_TX_DONE);
        GPIO_DAT_CLR(GPIO_TX_DONE);
    }
    
    len = uart_read(txdBuffer, TXD_BUFF_SIZE);
    if (len > 0)
    {
        txChnlBusy = true;
        DMA_UARTx_TX_CONF(DMA_CH_UART_TX, 1, txdBuffer, len, CCM_BASIC);
    }
}

void dmaUartSend(const uint8_t *data, uint16_t len)
{
    DMA_UARTx_TX_CONF(DMA_CH_UART_TX, 1, data, len, CCM_BASIC);

    while (!dma_chnl_done(DMA_CH_UART_TX)); // wait done
    //while (!(UART1->SR.TEM)); //Wait Transmitter Empty
}
#endif

void dmaTest(void)
{
    GPIO_DIR_SET_LO(GPIO_RUN | GPIO_TX_DONE | GPIO_RX_PING | GPIO_RX_PONG | GPIO_RX_RTOR);
    
    dma_init();
    
    // init dma chnl
    DMA_UARTx_TX_INIT(DMA_CH_UART_TX, 1);
    DMA_UARTx_RX_INIT(DMA_CH_UART_RX, 1);
    // config RX chnl
    DMA_UARTx_RX_CONF(DMA_CH_UART_RX, 1, &rxdBuffer[0], RXD_BUFF_HALF, CCM_PING_PONG);
    DMA_UARTx_RX_CONF(DMA_CH_UART_RX | DMA_CH_ALT, 1, &rxdBuffer[RXD_BUFF_HALF], RXD_BUFF_HALF, CCM_PING_PONG);
    
    #if !((DBG_MODE == 1) && (TEST_PORT == UART1_PORT))
    // init uart param
    uart_init(TEST_PORT, PA_UART_TX, PA_UART_RX);
    uart_conf(TEST_PORT, TEST_BAUD, TEST_LCRS);
    #endif

    uart_fctl(TEST_PORT, TEST_FCTL, TEST_RTOR, TEST_INTR); // RTO_EN
    uart_mctl(TEST_PORT, 1); // DMA_EN
    
    #if (DMA_IRQ_MODE)
    // enable irq
    DMACHCFG->IEFR0 = (1UL << DMA_CH_UART_RX) | (1UL << DMA_CH_UART_TX);
    NVIC_EnableIRQ(DMAC_IRQn);
    NVIC_EnableIRQ(UART1_IRQn);
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
