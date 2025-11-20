/**
 ****************************************************************************************
 *
 * @file uart_test.c
 *
 * @brief UART使用示例
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

/// UART端口和参数定义
#define TEST_PORT          UART1_PORT  // 测试使用的UART端口
#define TEST_BAUD          BRR_DIV(115200, 16M)  // 波特率配置：115200，基于16MHz时钟
#define TEST_LCRS          LCR_BITS(8, 1, none)  // 数据格式：8数据位，1停止位，无校验

/// 避免与调试UART冲突的GPIO引脚定义
#define PA_UART_TX         (6)   // UART发送引脚
#define PA_UART_RX         (7)   // UART接收引脚
#define PA_UART_RTS        (14)  // RTS（请求发送）引脚
#define PA_UART_CTS        (15)  // CTS（清除发送）引脚


/*
 * FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief UART初始化函数
 * 
 * @details 配置UART基本参数，包括引脚、波特率、数据格式等
 ****************************************************************************************
 */
static void uartInit(void)
{
    // 如果调试模式使用UART1且测试端口也是UART1，则跳过初始化以避免冲突
    #if !((DBG_MODE == 1) && (TEST_PORT == UART1_PORT))
    uart_init(TEST_PORT, PA_UART_TX, PA_UART_RX);  // 初始化UART引脚
    uart_conf(TEST_PORT, TEST_BAUD, TEST_LCRS);    // 配置UART参数
    #endif
    
    // 如果启用UART中断模式
    #if (UART_IRQ_MODE)
    /**
     * 配置UART FIFO控制和中断：
     * - FCR_FIFOEN_BIT: 使能FIFO功能
     * - FCR_RXTL_8BYTE: 接收FIFO触发阈值为8字节
     * - 超时值：20个字符时间
     * - UART_IR_RXRD_BIT: 使能接收数据就绪中断
     * - UART_IR_RTO_BIT: 使能接收超时中断
     */
    uart_fctl(TEST_PORT, FCR_FIFOEN_BIT | FCR_RXTL_8BYTE, 20, UART_IR_RXRD_BIT | UART_IR_RTO_BIT);
    #endif
    
    // 如果启用UART硬件流控制
    #if (UART_RTS_CTRL)
    uart_hwfc(TEST_PORT, PA_UART_RTS, PA_UART_CTS);  // 配置硬件流控制引脚
    #endif
}

/**
 ****************************************************************************************
 * @brief UART测试主函数
 * 
 * @details 实现UART回环测试：接收数据并立即发送回去
 * 
 * 工作流程：
 * 1. 初始化UART配置
 * 2. 进入无限循环
 * 3. 从UART接收一个字节数据
 * 4. 将接收到的数据通过UART发送出去
 * 5. 重复步骤3-4
 ****************************************************************************************
 */
void uartTest(void)
{
    uint8_t rx_data;  // 接收数据缓冲区
    
    uartInit();  // 初始化UART
    
    while (1)
    {
        // 回环测试：接收数据并立即发送回去
        rx_data = uart_getc(TEST_PORT);  // 从UART接收一个字节
        uart_putc(TEST_PORT, rx_data);   // 将接收到的字节发送回去
    }
}
