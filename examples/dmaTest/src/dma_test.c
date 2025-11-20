/**
 ****************************************************************************************
 *
 * @file dma_test.c
 *
 * @brief DMA UART收发传输演示程序
 *
 * @details
 * 本示例演示DMA在UART通信中的应用：
 * - 接收通道：Ping-Pong模式，双缓冲区连续接收
 * - 发送通道：Basic模式，按需发送数据
 * - 支持中断和轮询两种工作模式
 *
 * 工作原理：
 * 1. DMA接收：使用Ping-Pong模式在双半缓冲区间交替接收
 * 2. UART RTO：通过接收超时中断处理不完整数据包
 * 3. DMA发送：从接收缓冲区读取数据并发送
 * 4. 缓冲区管理：环形缓冲区实现数据存储和读取
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

#define DMA_IRQ_MODE       (1) ///< 工作模式选择: 1-中断模式, 0-轮询模式

/// DMA通道配置
#define DMA_CH_UART_RX     DMA_CH0 ///< UART接收通道，使用Ping-Pong模式
#define DMA_CH_UART_TX     DMA_CH1 ///< UART发送通道，使用Basic模式

/// UART端口和参数配置
#define TEST_PORT          0      ///< 0:UART1_PORT  1:UART2_PORT
#define TEST_BAUD          BRR_DIV(115200, 16M)  ///< 波特率配置
#define TEST_LCRS          LCR_BITS(8, 1, none)  ///< 数据位、停止位、校验位

#define TEST_RTOR          (100)  ///< 接收超时时间 (n)SYM=(n/10)字节
#define TEST_FCTL          (FCR_FIFOEN_BIT | FCR_RXTL_1BYTE)  ///< FIFO控制
#define TEST_INTR          (UART_IR_RTO_BIT)     ///< 使能接收超时中断

/// 避免与调试UART引脚冲突
#define PA_UART_TX         (6)    ///< UART发送引脚
#define PA_UART_RX         (7)    ///< UART接收引脚

/// 用于状态指示的GPIO引脚
#define GPIO_RUN           GPIO08 ///< 运行状态指示
#define GPIO_TX_DONE       GPIO09 ///< 发送完成指示
#define GPIO_RX_PING       GPIO10 ///< Ping缓冲区接收完成
#define GPIO_RX_PONG       GPIO11 ///< Pong缓冲区接收完成
#define GPIO_RX_RTOR       GPIO12 ///< 接收超时指示


/*
 * FUNCTIONS
 ****************************************************************************************
 */

/// *************** UART缓冲区管理函数 *************** 
#define TXD_BUFF_SIZE      (0x10)   ///< 发送缓冲区大小
#define RXD_BUFF_SIZE      (0x200)  ///< 接收缓冲区大小
#define RXD_BUFF_HALF      (RXD_BUFF_SIZE / 2) ///< Ping-Pong模式半缓冲区大小

volatile uint16_t rxdHead;          ///< 接收缓冲区头指针(写入位置)
volatile uint16_t rxdTail;          ///< 接收缓冲区尾指针(读取位置)
uint8_t rxdBuffer[RXD_BUFF_SIZE];   ///< 接收数据缓冲区
uint8_t txdBuffer[TXD_BUFF_SIZE];   ///< 发送数据缓冲区

/**
 ****************************************************************************************
 * @brief 获取接收缓冲区中可读数据长度
 *
 * @return uint16_t 可读数据字节数
 ****************************************************************************************
 */
uint16_t uart_size(void)
{
    return ((rxdHead + RXD_BUFF_SIZE - rxdTail) % RXD_BUFF_SIZE);
}

/**
 ****************************************************************************************
 * @brief 从接收缓冲区读取数据
 *
 * @param[out] buff 数据输出缓冲区
 * @param[in]  max  最大读取长度
 * @return uint16_t 实际读取长度
 *
 * @details
 * 实现环形缓冲区的数据读取，支持跨边界读取
 ****************************************************************************************
 */
uint16_t uart_read(uint8_t *buff, uint16_t max)
{
    uint16_t head = rxdHead;
    uint16_t tail = rxdTail;
    uint16_t tlen, len;
    
    if ((max == 0) || (head == tail))
    {
        return 0; // 缓冲区为空
    }

    // 计算可读数据长度
    len = (head + RXD_BUFF_SIZE - tail) % RXD_BUFF_SIZE;
    if (len > max) len = max;

    // 处理环形缓冲区边界情况
    if ((head > tail) || (tail + len <= RXD_BUFF_SIZE))
    {
        // 数据连续，直接拷贝
        memcpy(&buff[0], (const void *)&rxdBuffer[tail], len);
    }
    else
    {
        // 数据跨越边界，分段拷贝
        tlen = RXD_BUFF_SIZE - tail;

        memcpy(&buff[0], (const void *)&rxdBuffer[tail], tlen);     // 尾部数据
        memcpy(&buff[tlen], (const void *)&rxdBuffer[0], len - tlen); // 头部数据
    }
    rxdTail = (tail + len) % RXD_BUFF_SIZE;

    return len; // 返回实际读取长度
}

/// *************** DMA事件处理函数 *************** 
volatile bool rxChnlAlt  = false;   ///< 接收通道当前使用的缓冲区标识
volatile bool txChnlBusy = false;   ///< 发送通道忙状态标志

/**
 ****************************************************************************************
 * @brief UART接收DMA完成处理函数
 *
 * @details
 * 在Ping-Pong模式下，当半缓冲区接收完成时：
 * - 重新加载DMA配置继续接收
 * - 切换缓冲区标识
 * - 更新接收缓冲区头指针
 * - 产生脉冲信号指示完成
 ****************************************************************************************
 */
static void dmaUartRxDone(void)
{
    // 重新加载DMA配置，返回是否切换到备用通道
    rxChnlAlt = dma_chnl_reload(DMA_CH_UART_RX);
    
    if (rxChnlAlt)
    {
        // 切换到Pong缓冲区
        rxdHead = RXD_BUFF_HALF;  ///< 头指针指向后半部分
        // Ping缓冲区接收完成脉冲
        GPIO_DAT_SET(GPIO_RX_PING);
        GPIO_DAT_CLR(GPIO_RX_PING);
    }
    else
    {
        // 切换到Ping缓冲区
        rxdHead = 0;  ///< 头指针指向前半部分
        // Pong缓冲区接收完成脉冲
        GPIO_DAT_SET(GPIO_RX_PONG);
        GPIO_DAT_CLR(GPIO_RX_PONG);
    }
}

/**
 ****************************************************************************************
 * @brief UART接收超时处理函数
 *
 * @details
 * 当UART接收数据流中断超过设定时间时：
 * - 检查接收超时中断标志
 * - 清除中断标志
 * - 根据当前DMA状态计算已接收数据长度
 * - 更新接收缓冲区头指针
 ****************************************************************************************
 */
static void dmaUartRxRtor(void)
{
    #if (TEST_PORT == 0)
    uint32_t iflag = UART1->IFM.Word;  ///< 读取UART1中断标志
    #else
    uint32_t iflag = UART2->IFM.Word;  ///< 读取UART2中断标志
    #endif
    
    if (iflag & UART_IR_RTO_BIT)  ///< 检查接收超时中断
    {
        GPIO_DAT_SET(GPIO_RX_RTOR);  ///< 超时指示

        #if (TEST_PORT == 0)
            UART1->ICR.Word = UART_IR_RTO_BIT;  ///< 清除UART1超时中断
        #else
            UART2->ICR.Word = UART_IR_RTO_BIT;  ///< 清除UART2超时中断
        #endif
        
        // 根据当前DMA通道计算已接收数据长度
        if (rxChnlAlt)
        {
            // 当前使用备用通道(Pong)，计算已接收数据位置
            rxdHead = RXD_BUFF_HALF + (RXD_BUFF_HALF - dma_chnl_remain(DMA_CH_UART_RX | DMA_CH_ALT));
        }
        else
        {
            // 当前使用主通道(Ping)，计算已接收数据位置
            rxdHead = 0 + (RXD_BUFF_HALF - dma_chnl_remain(DMA_CH_UART_RX));
        }
        
        GPIO_DAT_CLR(GPIO_RX_RTOR);  ///< 清除超时指示
    }
}

#if (DMA_IRQ_MODE)
/**
 ****************************************************************************************
 * @brief DMA中断服务函数
 *
 * @details
 * 处理DMA通道完成中断：
 * - 读取中断标志寄存器
 * - 禁用相关中断
 * - 清除中断标志
 * - 根据通道标识调用相应处理函数
 * - 重新使能中断
 ****************************************************************************************
 */
void DMAC_IRQHandler(void)
{
    uint32_t iflag = DMACHCFG->IFLAG0;  ///< 读取DMA中断标志寄存器
    
    GPIO_DAT_SET(GPIO_RUN);  ///< 运行状态指示
    
    // 禁用已触发的中断
    DMACHCFG->IEFR0 &= ~iflag;
    // 清除中断标志
    DMACHCFG->ICFR0 = iflag;
    
    // 处理接收通道中断
    if (iflag & (1UL << DMA_CH_UART_RX))
    {
        dmaUartRxDone();
    }
    
    // 处理发送通道中断
    if (iflag & (1UL << DMA_CH_UART_TX))
    {
        txChnlBusy = false;  ///< 发送完成，清除忙状态
        GPIO_DAT_SET(GPIO_TX_DONE);
        GPIO_DAT_CLR(GPIO_TX_DONE);
    }
    
    // 重新使能中断
    DMACHCFG->IEFR0 |= iflag;
    
    GPIO_DAT_CLR(GPIO_RUN);  ///< 清除运行指示
}

/**
 ****************************************************************************************
 * @brief UART1中断服务函数
 ****************************************************************************************
 */
void UART1_IRQHandler(void)
{
    dmaUartRxRtor();
}

/**
 ****************************************************************************************
 * @brief UART2中断服务函数
 ****************************************************************************************
 */
void UART2_IRQHandler(void)
{
    dmaUartRxRtor();
}

/**
 ****************************************************************************************
 * @brief DMA UART主循环处理函数（中断模式）
 *
 * @details
 * 在中断模式下：
 * - 检查发送通道状态
 * - 从接收缓冲区读取数据
 * - 配置DMA发送数据
 ****************************************************************************************
 */
static void dmaUartLoop(void)
{
    uint16_t len;
    
    if (txChnlBusy)
    {
        return;  ///< 发送通道忙，等待完成
    }
    
    len = uart_read(txdBuffer, TXD_BUFF_SIZE);
    if (len > 0)
    {
        txChnlBusy = true;  ///< 设置发送忙状态
        
        #if (TEST_PORT == 0)
        DMA_UARTx_TX_CONF(DMA_CH_UART_TX, 1, txdBuffer, len, CCM_BASIC);  ///< 配置UART1发送
        #else
        DMA_UARTx_TX_CONF(DMA_CH_UART_TX, 2, txdBuffer, len, CCM_BASIC);  ///< 配置UART2发送
        #endif
    }
}

/**
 ****************************************************************************************
 * @brief DMA UART数据发送函数（中断模式）
 *
 * @param[in] data 待发送数据指针
 * @param[in] len  发送数据长度
 ****************************************************************************************
 */
void dmaUartSend(const uint8_t *data, uint16_t len)
{
    txChnlBusy = true;

    #if (TEST_PORT == 0)
    DMA_UARTx_TX_CONF(DMA_CH_UART_TX, 1, data, len, CCM_BASIC);  ///< 配置UART1发送
    #else
    DMA_UARTx_TX_CONF(DMA_CH_UART_TX, 2, data, len, CCM_BASIC);  ///< 配置UART2发送
    #endif
    
    while (txChnlBusy);  ///< 等待发送完成
    //while (!(UART1->SR.TEM)); // 等待发送器空
}

#else  // 轮询模式

/**
 ****************************************************************************************
 * @brief DMA轮询处理函数
 *
 * @details
 * 在轮询模式下：
 * - 检查接收通道完成状态
 * - 处理接收完成或接收超时
 ****************************************************************************************
 */
static void dmaPolling(void)
{
    if (dma_chnl_done(DMA_CH_UART_RX))  ///< 检查接收通道是否完成
    {
        GPIO_DAT_SET(GPIO_RUN);
        dmaUartRxDone();  ///< 处理接收完成
        GPIO_DAT_CLR(GPIO_RUN);
    }
    else
    {
        dmaUartRxRtor();  ///< 检查接收超时
    }
}

/**
 ****************************************************************************************
 * @brief DMA UART主循环处理函数（轮询模式）
 *
 * @details
 * 在轮询模式下：
 * - 检查发送通道状态
 * - 从接收缓冲区读取数据
 * - 配置DMA发送数据
 ****************************************************************************************
 */
static void dmaUartLoop(void)
{
    uint16_t len;
    
    if (txChnlBusy)
    {
        if (!dma_chnl_done(DMA_CH_UART_TX))  ///< 检查发送是否完成
        {
            return;  ///< 发送未完成，继续等待
        }
        
        txChnlBusy = false;  ///< 发送完成，清除忙状态
        GPIO_DAT_SET(GPIO_TX_DONE);
        GPIO_DAT_CLR(GPIO_TX_DONE);
    }
    
    len = uart_read(txdBuffer, TXD_BUFF_SIZE);
    if (len > 0)
    {
        txChnlBusy = true;  ///< 设置发送忙状态
        
        #if (TEST_PORT == 0)
        DMA_UARTx_TX_CONF(DMA_CH_UART_TX, 1, txdBuffer, len, CCM_BASIC);  ///< 配置UART1发送
        #else
        DMA_UARTx_TX_CONF(DMA_CH_UART_TX, 2, txdBuffer, len, CCM_BASIC);  ///< 配置UART2发送
        #endif
    }
}

/**
 ****************************************************************************************
 * @brief DMA UART数据发送函数（轮询模式）
 *
 * @param[in] data 待发送数据指针
 * @param[in] len  发送数据长度
 ****************************************************************************************
 */
void dmaUartSend(const uint8_t *data, uint16_t len)
{
    #if (TEST_PORT == 0)
    DMA_UARTx_TX_CONF(DMA_CH_UART_TX, 1, data, len, CCM_BASIC);  ///< 配置UART1发送
    #else
    DMA_UARTx_TX_CONF(DMA_CH_UART_TX, 2, data, len, CCM_BASIC);  ///< 配置UART2发送
    #endif
        
    while (!dma_chnl_done(DMA_CH_UART_TX));  ///< 轮询等待发送完成
    //while (!(UART1->SR.TEM)); // 等待发送器空
}

#endif

/**
 ****************************************************************************************
 * @brief DMA测试主函数
 *
 * @details
 * 执行DMA UART通信的完整测试流程：
 * 1. GPIO初始化：状态指示引脚
 * 2. DMA模块初始化
 * 3. UART和DMA通道配置
 * 4. 中断配置（中断模式）
 * 5. 主循环处理
 ****************************************************************************************
 */
void dmaTest(void)
{
    // 配置GPIO为输出模式，用于状态指示
    GPIO_DIR_SET_LO(GPIO_RUN | GPIO_TX_DONE | GPIO_RX_PING | GPIO_RX_PONG | GPIO_RX_RTOR);
    
    dma_init();  ///< 初始化DMA模块
    
    #if (TEST_PORT == 0)
    // 初始化DMA通道
    DMA_UARTx_TX_INIT(DMA_CH_UART_TX, 1);  ///< 初始化UART1发送通道
    DMA_UARTx_RX_INIT(DMA_CH_UART_RX, 1);  ///< 初始化UART1接收通道
    
    // 配置接收通道为Ping-Pong模式
    DMA_UARTx_RX_CONF(DMA_CH_UART_RX, 1, &rxdBuffer[0], RXD_BUFF_HALF, CCM_PING_PONG);         ///< Ping缓冲区
    DMA_UARTx_RX_CONF(DMA_CH_UART_RX | DMA_CH_ALT, 1, &rxdBuffer[RXD_BUFF_HALF], RXD_BUFF_HALF, CCM_PING_PONG); ///< Pong缓冲区
    #else
    // 初始化DMA通道
    DMA_UARTx_TX_INIT(DMA_CH_UART_TX, 2);  ///< 初始化UART2发送通道
    DMA_UARTx_RX_INIT(DMA_CH_UART_RX, 2);  ///< 初始化UART2接收通道
    
    // 配置接收通道为Ping-Pong模式
    DMA_UARTx_RX_CONF(DMA_CH_UART_RX, 2, &rxdBuffer[0], RXD_BUFF_HALF, CCM_PING_PONG);         ///< Ping缓冲区
    DMA_UARTx_RX_CONF(DMA_CH_UART_RX | DMA_CH_ALT, 2, &rxdBuffer[RXD_BUFF_HALF], RXD_BUFF_HALF, CCM_PING_PONG); ///< Pong缓冲区
    #endif
    
    #if !((DBG_MODE == 1) && (TEST_PORT == 0))
    // 初始化UART参数
    uart_init(TEST_PORT, PA_UART_TX, PA_UART_RX);     ///< 初始化UART引脚
    uart_conf(TEST_PORT, TEST_BAUD, TEST_LCRS);       ///< 配置UART参数
    #endif

    uart_fctl(TEST_PORT, TEST_FCTL, TEST_RTOR, TEST_INTR);  ///< 配置FIFO和接收超时
    uart_mctl(TEST_PORT, 1);  ///< 使能UART DMA模式
    
    #if (DMA_IRQ_MODE)
    // 使能DMA中断
    DMACHCFG->IEFR0 = (1UL << DMA_CH_UART_RX) | (1UL << DMA_CH_UART_TX);  ///< 使能接收和发送通道中断
    NVIC_EnableIRQ(DMAC_IRQn);  ///< 使能DMA控制器中断
    
    #if (TEST_PORT == 0)
    NVIC_EnableIRQ(UART1_IRQn);  ///< 使能UART1中断
    #else
    NVIC_EnableIRQ(UART2_IRQn);  ///< 使能UART2中断
    #endif
    
    __enable_irq();  ///< 全局使能中断
    #endif
    
    while (1)
    {
        #if !(DMA_IRQ_MODE)
        dmaPolling();  ///< 轮询模式下的DMA状态检查
        #endif
        dmaUartLoop();  ///< 主循环处理
    }
}
