/**
 ****************************************************************************************
 *
 * @file spim_test.c
 *
 * @brief SPI Master使用示例
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "drvs.h"
#include "regs.h"
#include "dbg.h"


/*
 * DEFINES
 ****************************************************************************************
 */

#define BUFF_SIZE              128 ///< 缓冲区大小

uint8_t tx_buff[BUFF_SIZE];    ///< 发送数据缓冲区
uint8_t rx_buff[BUFF_SIZE];    ///< 接收数据缓冲区


/*
 * FUNCTIONS
 ****************************************************************************************
 */

#if (SPI_DMA_MODE)    ///< DMA模式

#define DMA_CHNL_TX            0   ///< DMA发送通道
#define DMA_CHNL_RX            1   ///< DMA接收通道

#if (SPI_FLASH_OP)    ///< SPI Flash操作
/**
 * @brief SPI Master处理函数（Flash操作）
 * 
 * 演示SPI Flash的常见操作：
 * - 读取Flash ID
 * - 读取状态寄存器
 * - 擦除操作
 * - 写使能
 * - 数据写入和读取
 */
static void spimProc(void)
{ 
    // SPI Master测试
    debug("Read FlashID(cmd:0x9F, rxlen:3)\r\n");
    tx_buff[0] = 0x9F;     ///< Flash ID读取命令
    dma_chnl_ctrl(DMA_CHNL_TX, CHNL_EN); ///< 使能SPI DMA TX通道
    
    // 配置DMA传输：发送1字节命令+3字节ID，接收1+3字节数据
    DMA_SPIM_TX_CONF(DMA_CHNL_TX, tx_buff, 1+3, CCM_BASIC);
    DMA_SPIM_RX_CONF(DMA_CHNL_RX, rx_buff, 1+3, CCM_BASIC);
    
    SPIM_CS_L(SPI_CS_PAD);     ///< 拉低CS信号，选择从设备
    spim_begin(1+3);           ///< 启动SPI传输，设置数据长度为4字节
    spim_wait();               ///< 等待传输完成
    SPIM_CS_H(SPI_CS_PAD);     ///< 拉高CS信号，取消选择从设备
    
    dma_chnl_ctrl(DMA_CHNL_TX, CHNL_DIS); ///< 禁用SPI DMA TX通道
    debugHex(rx_buff, 4);      ///< 打印接收到的4字节数据

    debug("Flash Status(cmd:0x05, rxlen:1)\r\n");
    tx_buff[0] = 0x05;         ///< 状态寄存器读取命令
    dma_chnl_ctrl(DMA_CHNL_TX, CHNL_EN);
    DMA_SPIM_TX_CONF(DMA_CHNL_TX, tx_buff, 1+1, CCM_BASIC);
    DMA_SPIM_RX_CONF(DMA_CHNL_RX, rx_buff, 1+1, CCM_BASIC);
    SPIM_CS_L(SPI_CS_PAD);
    spim_begin(1+1);
    spim_wait();
    SPIM_CS_H(SPI_CS_PAD);
    dma_chnl_ctrl(DMA_CHNL_TX, CHNL_DIS);
    debugHex(rx_buff, 2);

    debug("Flash ER(cmd:0x60, rxlen:1)\r\n");
    tx_buff[0] = 0x60;         ///< 芯片擦除命令
    dma_chnl_ctrl(DMA_CHNL_TX, CHNL_EN);
    DMA_SPIM_TX_CONF(DMA_CHNL_TX, tx_buff, 1, CCM_BASIC);
    DMA_SPIM_RX_CONF(DMA_CHNL_RX, rx_buff, 1, CCM_BASIC);
    SPIM_CS_L(SPI_CS_PAD);
    spim_begin(1);
    spim_wait();
    SPIM_CS_H(SPI_CS_PAD);
    dma_chnl_ctrl(DMA_CHNL_TX, CHNL_DIS);
    debugHex(rx_buff, 1);
    
    debug("Flash WR EN(cmd:0x06, rxlen:1)\r\n");
    tx_buff[0] = 0x06;         ///< 写使能命令
    dma_chnl_ctrl(DMA_CHNL_TX, CHNL_EN);
    DMA_SPIM_TX_CONF(DMA_CHNL_TX, tx_buff, 1, CCM_BASIC);
    DMA_SPIM_RX_CONF(DMA_CHNL_RX, rx_buff, 1, CCM_BASIC);
    SPIM_CS_L(SPI_CS_PAD);
    spim_begin(1);
    spim_wait();
    SPIM_CS_H(SPI_CS_PAD);
    dma_chnl_ctrl(DMA_CHNL_TX, CHNL_DIS);
    debugHex(rx_buff, 1);
    
    debug("Flash WR(cmd:0x02, adr:0x000000, datlen:8)\r\n");
    tx_buff[0] = 0x02;         ///< 页编程命令
    tx_buff[1] = 0x00;         ///< 地址字节2
    tx_buff[2] = 0x00;         ///< 地址字节1
    tx_buff[3] = 0x00;         ///< 地址字节0
    uint8_t data_len = BUFF_SIZE - 4; ///< 计算数据长度
    
    // 填充测试数据
    for (uint8_t idx = 0; idx < data_len; idx++)
    {
        tx_buff[4+idx] = idx+1;
    }
    
    dma_chnl_ctrl(DMA_CHNL_TX, CHNL_EN);
    DMA_SPIM_TX_CONF(DMA_CHNL_TX, tx_buff, 4+data_len, CCM_BASIC);
    DMA_SPIM_RX_CONF(DMA_CHNL_RX, rx_buff, 4+data_len, CCM_BASIC);
    SPIM_CS_L(SPI_CS_PAD);
    spim_begin(4+data_len);
    spim_wait();
    SPIM_CS_H(SPI_CS_PAD);
    dma_chnl_ctrl(DMA_CHNL_TX, CHNL_DIS);
    debugHex(rx_buff, 4+data_len);
    memset(tx_buff, 0x00, BUFF_SIZE);  ///< 清空发送缓冲区
    
    debug("Flash RD(cmd:0x03, adr:0x000000, datlen:%d)\r\n", data_len);
    tx_buff[0] = 0x03;         ///< 数据读取命令
    tx_buff[1] = 0x00;
    tx_buff[2] = 0x00;
    tx_buff[3] = 0x00;
    dma_chnl_ctrl(DMA_CHNL_TX, CHNL_EN);
    DMA_SPIM_TX_CONF(DMA_CHNL_TX, tx_buff, 4+data_len, CCM_BASIC);
    DMA_SPIM_RX_CONF(DMA_CHNL_RX, rx_buff, 4+data_len, CCM_BASIC);
    SPIM_CS_L(SPI_CS_PAD);
    spim_begin(4+data_len);
    spim_wait();
    SPIM_CS_H(SPI_CS_PAD);
    dma_chnl_ctrl(DMA_CHNL_TX, CHNL_DIS);
    debugHex(rx_buff, BUFF_SIZE);
    
    // 持续测试循环
    while (1)
    {
        // 随机长度测试
        data_len = (sadc_rand_num())%(BUFF_SIZE - 4);
        debug("Flash RD(cmd:0x03, adr:0x000000, datlen:%d)\r\n", data_len);
        tx_buff[0] = 0x03;
        tx_buff[1] = 0x00;
        tx_buff[2] = 0x00;
        tx_buff[3] = 0x00;
        dma_chnl_ctrl(DMA_CHNL_TX, CHNL_EN);
        DMA_SPIM_TX_CONF(DMA_CHNL_TX, tx_buff, 4+data_len, CCM_BASIC);
        DMA_SPIM_RX_CONF(DMA_CHNL_RX, rx_buff, 4+data_len, CCM_BASIC);
        SPIM_CS_L(SPI_CS_PAD);
        spim_begin(4+data_len);
        spim_wait();
        SPIM_CS_H(SPI_CS_PAD);
        dma_chnl_ctrl(DMA_CHNL_TX, CHNL_DIS);
        debugHex(rx_buff, 4+data_len);
        
        // 验证接收数据正确性
        for (uint8_t idx = 0; idx < data_len; idx++)
        {
            if(rx_buff[4+idx] != idx + 1)
                while(1);  ///< 数据错误时进入死循环
        }
        
        memset(rx_buff, 0, BUFF_SIZE);  ///< 清空接收缓冲区
    };
}
#else
/**
 * @brief SPI Master处理函数（普通数据传输）
 * 
 * 演示普通的SPI全双工数据传输
 */
void spimProc(void)
{
    uint8_t tx_data = 0xFF; ///< 发送数据起始值
    uint8_t rx_data = 0xFF; ///< 期望接收数据起始值
    
    while (1)
    {
        // 填充发送缓冲区
        for (uint8_t i = 0; i < BUFF_SIZE; i++)
        {
            tx_buff[i] = tx_data++;
        }
        
        dma_chnl_ctrl(DMA_CHNL_TX, CHNL_EN); ///< 使能SPI DMA TX通道
        
        // 配置DMA传输
        DMA_SPIM_TX_CONF(DMA_CHNL_TX, tx_buff, BUFF_SIZE, CCM_BASIC);
        DMA_SPIM_RX_CONF(DMA_CHNL_RX, rx_buff, BUFF_SIZE, CCM_BASIC);
        
        SPIM_CS_L(SPI_CS_PAD);
        spim_begin(BUFF_SIZE);
        spim_wait();
        SPIM_CS_H(SPI_CS_PAD);
        
        dma_chnl_ctrl(DMA_CHNL_TX, CHNL_DIS); ///< 禁用SPI DMA TX通道
       
        debug("TX: %02X ~ %02X, RX:\r\n", tx_buff[0], tx_buff[BUFF_SIZE-1]);
        debugHex(rx_buff, BUFF_SIZE);
        
        // 验证接收数据
        for (uint8_t i = 0; i < BUFF_SIZE; i++)
        {
            if (rx_buff[i] != rx_data)
            {
                debug("RX Wrong(curr:%02X, expt:%02X)\r\n", rx_buff[i], rx_data);
                while(1);  ///< 数据错误时进入死循环
            }
            rx_data++;
        }
    }
}
#endif

/**
 * @brief SPI Master初始化（DMA模式）
 * 
 * 初始化DMA控制器和SPI Master，配置为DMA工作模式
 */
static void spimInit(void)
{
    dma_init();     ///< 初始化DMA控制器
    
    // SPI Master初始化
    SPIM_CS_INIT(SPI_CS_PAD);   ///< 初始化CS引脚为GPIO输出
    spim_init(SPI_CLK_PAD, SPI_MISO_PAD, SPI_MOSI_PAD); ///< 初始化SPI引脚
    
    // SPI Master配置（DMA模式）
    spim_conf(SPIM_CR_DFLT | SPIM_CR_RX_DMA_BIT | SPIM_CR_TX_DMA_BIT); ///< CTRL寄存器：使能DMA模式
    
    // DMA通道初始化
    DMA_SPIM_TX_INIT(DMA_CHNL_TX);  ///< 初始化SPI发送DMA通道
    DMA_SPIM_RX_INIT(DMA_CHNL_RX);  ///< 初始化SPI接收DMA通道
}

#else //(SPIM_MCU_MODE)   ///< MCU模式

#if (SPI_FLASH_OP)
#define HALF_DUPLEX        1   ///< 半双工模式标志

/**
 * @brief SPI Master处理函数（Flash操作，MCU模式）
 * 
 * 使用MCU模式进行SPI Flash操作，支持半双工和全双工模式
 */
static void spimProc(void)
{
    // SPI Master测试
    debug("Read FlashID(cmd:0x9F, rxlen:3)\r\n");
    tx_buff[0] = 0x9F;     ///< Flash ID读取命令
    SPIM_CS_L(SPI_CS_PAD);
    
    #if (HALF_DUPLEX)
    spim_halfdx(tx_buff, 1, rx_buff, 3);   ///< 半双工模式：先发送1字节，再接收3字节
    #else
    spim_duplex(tx_buff, rx_buff, 1 + 3);  ///< 全双工模式：同时收发4字节
    #endif
    
    SPIM_CS_H(SPI_CS_PAD);
    debugHex(rx_buff, 4);
    
    #if (SPIM_FSH_WR)  ///< Flash写操作
    debug("Flash WR_EN(cmd:0x06, rxlen:0)\r\n");
    tx_buff[0] = 0x06;     ///< 写使能命令
    SPIM_CS_L(SPI_CS_PAD);
    
    #if (HALF_DUPLEX)
    spim_halfdx(tx_buff, 1, rx_buff, 0);   ///< 只发送不接收
    #else
    spim_duplex(tx_buff, rx_buff, 1);      ///< 发送1字节
    #endif
    
    SPIM_CS_H(SPI_CS_PAD);
    debugHex(rx_buff, 1);

    debug("Flash WR(cmd:0x02, adr:0x000000, datlen:4)\r\n");
    tx_buff[0] = 0x02;     ///< 页编程命令
    tx_buff[1] = 0x04;     ///< 地址
    tx_buff[2] = 0x00;
    tx_buff[3] = 0x00;
    SPIM_CS_L(SPI_CS_PAD);
    
    #if (HALF_DUPLEX)
    spim_halfdx(tx_buff, 8, rx_buff, 0);   ///< 发送8字节数据
    #else
    spim_duplex(tx_buff, rx_buff, 1+3+4);  ///< 发送命令+地址+数据
    #endif
    
    SPIM_CS_H(SPI_CS_PAD);
    debugHex(rx_buff, 8);
    
    bootDelayMs(10);       ///< 等待Flash操作完成
    #endif
    
    debug("Flash Status(cmd:0x05, rxlen:1)\r\n");
    tx_buff[0] = 0x05;     ///< 状态寄存器读取命令
    SPIM_CS_L(SPI_CS_PAD);
    
    #if (HALF_DUPLEX)
    spim_halfdx(tx_buff, 1, rx_buff, 1);   ///< 发送1字节命令，接收1字节状态
    #else
    spim_duplex(tx_buff, rx_buff, 1 + 1);  ///< 同时收发2字节
    #endif
    
    SPIM_CS_H(SPI_CS_PAD);
    debugHex(rx_buff, 2);
    
    debug("Flash RD(cmd:0x03, adr:0x000000, datlen:8)\r\n");
    tx_buff[0] = 0x03;     ///< 数据读取命令
    tx_buff[1] = 0x00;     ///< 地址
    tx_buff[2] = 0x00;
    tx_buff[3] = 0x00;
    SPIM_CS_L(SPI_CS_PAD);
    
    #if (HALF_DUPLEX)
    spim_halfdx(tx_buff, 4, rx_buff, 8);   ///< 发送4字节地址，接收8字节数据
    #else
    spim_duplex(tx_buff, rx_buff, 1+3+8);  ///< 同时收发12字节
    #endif
    
    SPIM_CS_H(SPI_CS_PAD);
    debugHex(rx_buff, 8);
    
    while (1)
    {
        // 空循环
    };
}
#else
/**
 * @brief SPI Master处理函数（普通数据传输，MCU模式）
 * 
 * 使用MCU模式进行普通的SPI全双工数据传输
 */
void spimProc(void)
{
    uint8_t tx_data = 0xFF; ///< 发送数据起始值
    uint8_t rx_data = 0xFF; ///< 期望接收数据起始值
    
    while (1)
    {
        // 填充发送缓冲区
        for (uint8_t i = 0; i < BUFF_SIZE; i++)
        {
            tx_buff[i] = tx_data++;
        }
        
        SPIM_CS_L(SPI_CS_PAD);
        spim_duplex(tx_buff, rx_buff, BUFF_SIZE);  ///< 全双工数据传输
        SPIM_CS_H(SPI_CS_PAD);
        
        debug("TX: %02X ~ %02X, RX:\r\n", tx_buff[0], tx_buff[BUFF_SIZE-1]);
        debugHex(rx_buff, BUFF_SIZE);
        
        // 验证接收数据
        for (uint8_t i = 0; i < BUFF_SIZE; i++)
        {
            if (rx_buff[i] != rx_data)
            {
                debug("RX Wrong(curr:%02X, expt:%02X)\r\n", rx_buff[i], rx_data);
                while(1);  ///< 数据错误时进入死循环
            }
            rx_data++;
        }
    }
}
#endif

/**
 * @brief SPI Master初始化（MCU模式）
 * 
 * 初始化SPI Master，配置为MCU工作模式
 */
static void spimInit(void)
{
    // SPI Master初始化
    SPIM_CS_INIT(SPI_CS_PAD);   ///< 初始化CS引脚为GPIO输出
    spim_init(SPI_CLK_PAD, SPI_MISO_PAD, SPI_MOSI_PAD); ///< 初始化SPI引脚

    // SPI Master配置
    spim_conf(SPIM_CR_DFLT);    ///< CTRL寄存器：使用默认MCU模式配置
}

#endif

/**
 * @brief SPI Master测试主函数
 * 
 * 初始化缓冲区，执行SPI初始化，运行SPI处理函数
 */
void spimTest(void)
{
    debug("spimTest Start...\r\n");
    
    // 初始化缓冲区
    for (uint8_t idx = 0; idx < BUFF_SIZE; idx++)
    {
        tx_buff[idx] = 0xFF;    ///< 发送缓冲区填充0xFF
        rx_buff[idx] = 0x00;    ///< 接收缓冲区清零
    }
    
    spimInit();     ///< 执行SPI初始化
    
    spimProc();     ///< 运行SPI处理函数
}
