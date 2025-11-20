/**
 ****************************************************************************************
 *
 * @file uart.c
 *
 * @brief UART 驱动程序
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

/// 获取 UARTx 指针，'port' : 0-UART1, 1-UART2
#if defined(UART1_BASE)
#define UART_PTR(port)         ((UART_TypeDef *)(UART1_BASE + (port) * 0x1000))
#else
#define UART_PTR(port)         ((port == 0) ? UART1 : UART2)
#endif


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 * @brief 初始化UART端口
 * @param port UART端口号 (0: UART1, 1: UART2)
 * @param io_tx 发送引脚
 * @param io_rx 接收引脚
 */
void uart_init(uint8_t port, uint8_t io_tx, uint8_t io_rx)
{
    iom_ctrl(io_rx, IOM_SEL_CSC | IOM_PULLUP | IOM_INPUT);    /* 配置接收引脚为CSC功能、上拉、输入模式 */
    iom_ctrl(io_tx, IOM_SEL_CSC | IOM_PULLUP | IOM_DRV_LVL1); /* 配置发送引脚为CSC功能、上拉、驱动电平1 */

    //iocsc_uart(port, io_tx, io_rx);
    csc_output(io_tx, CSC_UART1_TXD + port * 2);  /* 配置发送引脚CSC输出 */
    csc_input(io_rx,  CSC_UART1_RXD + port * 2);  /* 配置接收引脚CSC输入 */

    // uart_clk_en rst_req
    RCC_APBCLK_EN(1 << (RCC_UART1_CLKEN_RUN_POS + port));   /* 使能UART时钟 */
    RCC_APBRST_REQ(1 << (RCC_UART1_RSTREQ_POS + port));     /* 请求UART复位 */

    uint8_t delay_cnt = 0x10;
    // Delay for Uart busy
    while (delay_cnt--)  //220ns/cnt(64Mhz test need delay 906ns)  --20231204 WHL
    {
        if ((GPIO->PIN >> io_rx) & 0x01)  /* 检查接收引脚电平 */
            return;
    }
}

/**
 * @brief 反初始化UART端口
 * @param port UART端口号 (0: UART1, 1: UART2)
 */
void uart_deinit(uint8_t port)
{
    // uart_clk_dis rst_req
    RCC_APBCLK_DIS(1 << (RCC_UART1_CLKEN_RUN_POS + port));  /* 禁用UART时钟 */
    RCC_APBRST_REQ(1 << (RCC_UART1_RSTREQ_POS + port));     /* 请求UART复位 */
}

/**
 * @brief 配置UART硬件流控制
 * @param port UART端口号 (0: UART1, 1: UART2)
 * @param io_rts RTS引脚
 * @param io_cts CTS引脚
 */
void uart_hwfc(uint8_t port, uint8_t io_rts, uint8_t io_cts)
{
    UART_TypeDef* uart = UART_PTR(port);

    //iocsc_uart_hwfc(port, io_rts, io_cts);
    csc_output(io_rts, CSC_UART1_RTS + port);  /* 配置RTS引脚CSC输出 */
    csc_input(io_cts,  CSC_UART1_CTS + port);  /* 配置CTS引脚CSC输入 */

    iom_ctrl(io_rts, IOM_SEL_CSC | IOM_DRV_LVL1 | IOM_PULLUP); /* 配置RTS引脚为CSC功能、驱动电平1、上拉 */
    iom_ctrl(io_cts, IOM_SEL_CSC | IOM_PULLUP   | IOM_INPUT);  /* 配置CTS引脚为CSC功能、上拉、输入模式 */

    // enable auto ctrl bit[3:2] (.AFCEN=1, .RTSCTRL=1)
    uart->MCR.Word |= 0x0C;  /* MCR: 使能自动流控制(AFCEN)和RTS控制(RTSCTRL) */
}

/**
 * @brief 配置UART通信参数
 * @param port UART端口号 (0: UART1, 1: UART2)
 * @param cfg_BRR 波特率配置值
 * @param cfg_LCR 线路控制寄存器配置
 */
void uart_conf(uint8_t port, uint16_t cfg_BRR, uint16_t cfg_LCR)
{
    UART_TypeDef* uart = UART_PTR(port);

    // clear en
    uart->LCR.Word   = 0;  /* LCR: 清除线路控制寄存器 */

    // update BaudRate
    uart->LCR.BRWEN  = 1;      /* LCR: 使能波特率更新 */
    uart->BRR        = cfg_BRR; /* BRR: 设置波特率分频值 */
    uart->LCR.BRWEN  = 0;      /* LCR: 禁用波特率更新 */

    // enable fifo mode, reset
    uart->FCR.Word |= 0x07; /* FCR: 使能FIFO(FIFOEN)、复位接收FIFO(RFRST)、复位发送FIFO(TFRST) */

    // config params, enable
    uart->LCR.Word = cfg_LCR | LCR_RXEN_BIT;  /* LCR: 配置通信参数并启用接收 */
}

/**
 * @brief 配置UART FIFO控制和中断
 * @param port UART端口号 (0: UART1, 1: UART2)
 * @param fifo_ctl FIFO控制配置
 * @param bits_rto 接收超时配置
 * @param intr_en 中断使能配置
 */
void uart_fctl(uint8_t port, uint8_t fifo_ctl, uint16_t bits_rto, uint16_t intr_en)
{
    UART_TypeDef* uart = UART_PTR(port);

    uart->FCR.Word  = fifo_ctl; /* FCR: 配置FIFO控制 */
    uart->RTOR.Word = bits_rto; /* RTOR: 设置接收超时值 */
    uart->LCR.RTOEN = (bits_rto) ? 1 : 0;  /* LCR: 使能/禁用接收超时 */

    uart->IDR.Word  = UART_IR_ALL_MSK;  /* IDR: 禁用所有中断 */
    uart->IER.Word  = intr_en;          /* IER: 使能指定中断 */
}

/**
 * @brief 配置UART DMA控制
 * @param port UART端口号 (0: UART1, 1: UART2)
 * @param dma DMA使能标志
 */
void uart_mctl(uint8_t port, uint8_t dma)
{
    UART_TypeDef *uart = UART_PTR(port);

    uart->MCR.DMAEN = dma;      /* MCR: 使能/禁用DMA */
    uart->LCR.RTO_SEL = dma;    /* LCR: 设置RTO选择模式 */
}

/**
 * @brief 发送单个字符
 * @param port UART端口号 (0: UART1, 1: UART2)
 * @param ch 要发送的字符
 */
void uart_putc(uint8_t port, uint8_t ch)
{
    UART_TypeDef *uart = UART_PTR(port);

    while (!(uart->SR.TBEM));  /* SR: 等待发送缓冲区空 */
    uart->TBR = ch;            /* TBR: 写入发送数据 */
}

/**
 * @brief 接收单个字符
 * @param port UART端口号 (0: UART1, 1: UART2)
 * @return 接收到的字符
 */
uint8_t uart_getc(uint8_t port)
{
    UART_TypeDef *uart = UART_PTR(port);

    while (!(uart->SR.DR));    /* SR: 等待数据就绪 */
    return (uint8_t)(uart->RBR);  /* RBR: 读取接收数据 */
}

/**
 * @brief 等待UART发送完成
 * @param port UART端口号 (0: UART1, 1: UART2)
 */
void uart_wait(uint8_t port)
{
    UART_TypeDef *uart = UART_PTR(port);

    while (!(uart->SR.TBEM)); /* SR: 等待发送缓冲区空 */
    while (uart->SR.BUSY);    /* SR: 等待UART空闲 */
}

/**
 * @brief 发送数据块
 * @param port UART端口号 (0: UART1, 1: UART2)
 * @param len 数据长度
 * @param data 数据指针
 */
void uart_send(uint8_t port, uint16_t len, const uint8_t *data)
{
    UART_TypeDef *uart = UART_PTR(port);

    while (len--)
    {
        while (!(uart->SR.TBEM));  /* SR: 等待发送缓冲区空 */
        uart->TBR = *data++;       /* TBR: 发送数据 */
    }

//    while (!(uart->SR.TBEM)); // wait tx finish
//    while (uart->SR.BUSY);    // wait idle state
}
