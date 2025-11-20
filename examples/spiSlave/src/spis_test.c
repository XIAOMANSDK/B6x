/**
 ****************************************************************************************
 *
 * @file spis_test.c
 *
 * @brief SPI从设备使用示例
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "drvs.h"
#include "dbg.h"


#if (SPI_DMA_MODE)  // DMA模式配置

/// DMA通道定义
#define DMA_CHNL_TX            0  // 发送DMA通道
#define DMA_CHNL_RX            1  // 接收DMA通道

#define BUFF_SIZE              16  // 缓冲区大小

uint8_t tx_buff[BUFF_SIZE];  // 发送数据缓冲区
uint8_t rx_buff[BUFF_SIZE];  // 接收数据缓冲区

/**
 ****************************************************************************************
 * @brief SPI从设备DMA模式处理函数
 * 
 * @details 使用DMA自动处理SPI数据传输，实现高效的全双工通信
 ****************************************************************************************
 */
static void spisProc(void)
{
    // 从设备连接时自动发送0xFF，因此主设备先发送0xFF，然后TX/RX保持相同数据序列
    uint8_t tx_data = 0x00;  // 发送数据起始值
    uint8_t rx_data = 0xFF;  // 接收数据期望起始值

    // 初始化发送缓冲区数据
    for (uint8_t i = 0; i < BUFF_SIZE; i++)
    {
        tx_buff[i] = tx_data++;
    }
    
    // 配置DMA通道：接收从SPIS->RX_DAT到rx_buff，发送从tx_buff到SPIS->TX_DAT
    DMA_SPIS_RX_CONF(DMA_CHNL_RX, rx_buff, BUFF_SIZE, CCM_BASIC);  // 基本循环控制模式
    DMA_SPIS_TX_CONF(DMA_CHNL_TX, tx_buff, BUFF_SIZE, CCM_BASIC);
    
    debug("DMA Remain(TX:%d,RX:%d)\r\n", dma_chnl_remain(DMA_CHNL_TX), dma_chnl_remain(DMA_CHNL_RX));
    
    // 主处理循环
    while (1)
    {
        // 检查接收DMA是否完成
        if (dma_chnl_done(DMA_CHNL_RX))
        {
            // 验证接收数据是否正确
            for (uint8_t i = 0; i < BUFF_SIZE; i++)
            {
                if (rx_buff[i] != rx_data)
                {
                    debug("RX Wrong(curr:%02X, expt:%02X)\r\n", rx_buff[i], rx_data);
                    debugHex(rx_buff, BUFF_SIZE);
                    while (1);  // 数据错误，进入死循环
                }
                rx_data++;
            }
            
            // 重新加载DMA配置，继续下一次传输
            dma_chnl_reload(DMA_CHNL_RX);
        }
        
        // 检查发送DMA是否完成
        if (dma_chnl_done(DMA_CHNL_TX))
        {
            // 更新发送缓冲区数据
            for (uint8_t i = 0; i < BUFF_SIZE; i++)
            {
                tx_buff[i] = tx_data++;
            }
            
            // 重新加载DMA配置，继续下一次传输
            dma_chnl_reload(DMA_CHNL_TX);
        }
    }
}

/**
 ****************************************************************************************
 * @brief SPI从设备DMA模式初始化
 ****************************************************************************************
 */
static void spisInit(void)
{
    dma_init();  // 初始化DMA控制器

    // SPI从设备引脚初始化：CS、CLK、MISO、MOSI
    spis_init(SPI_CS_PAD, SPI_CLK_PAD, SPI_MISO_PAD, SPI_MOSI_PAD);  
    
    // SPI从设备配置：默认配置 + TX/RX DMA使能
    spis_conf(SPIS_CR_DFLT | SPIS_CR_TX_DMA_BIT | SPIS_CR_RX_DMA_BIT);
    
    // DMA通道初始化
    DMA_SPIS_TX_INIT(DMA_CHNL_TX);  // 发送通道连接到SPIS TX外设
    DMA_SPIS_RX_INIT(DMA_CHNL_RX);  // 接收通道连接到SPIS RX外设
}

#else //(SPIM_MCU_MODE)  // MCU模式配置

/**
 ****************************************************************************************
 * @brief SPI从设备MCU模式处理函数
 * 
 * @details 通过CPU轮询方式处理SPI数据传输，适合低速或简单应用
 ****************************************************************************************
 */
static void spisProc(void)
{
    // 从设备连接时自动发送0xFF，因此主设备先发送0xFF，然后TX/RX保持相同数据序列
    uint8_t tx_data = 0x00;  // 发送数据起始值
    uint8_t rx_data = 0xFF;  // 接收数据期望起始值
    uint8_t rx_curr;         // 当前接收数据

    debug("SPI getc & putc\r\n"); 
    
    // 主处理循环
    while (1)
    {
        // 尝试接收一个字节
        if (spis_getc(&rx_curr))
        {
            // 验证接收数据是否正确
            if (rx_curr != rx_data)
            {
                debug("RX Wrong(curr:%02X, expt:%02X), TX:%02X\r\n", rx_curr, rx_data, tx_data);
                while (1);  // 数据错误，进入死循环
            }
            
            rx_data++;  // 更新期望接收值
        }
        
        // 发送一个字节数据
        spis_putc(tx_data++);
    }
}

/**
 ****************************************************************************************
 * @brief SPI从设备MCU模式初始化
 ****************************************************************************************
 */
static void spisInit(void)
{
    // SPI从设备引脚初始化：CS、CLK、MISO、MOSI
    spis_init(SPI_CS_PAD, SPI_CLK_PAD, SPI_MISO_PAD, SPI_MOSI_PAD);  
    
    // SPI从设备配置：使用默认配置
    spis_conf(SPIS_CR_DFLT);
}

#endif

/**
 ****************************************************************************************
 * @brief SPI从设备测试主函数
 ****************************************************************************************
 */
void spisTest(void)
{
    debug("spisTest Start...\r\n");
    
    spisInit();  // SPI从设备初始化
    
    spisProc();  // 执行SPI从设备处理
}
