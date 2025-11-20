/**
 ****************************************************************************************
 *
 * @file i2c_test.c
 *
 * @brief I2C主从设备使用示例
 *
 * @details
 * 本文件演示I2C主从两种工作模式：
 * - 主模式：作为控制器访问EEPROM设备
 * - 从模式：响应主设备的读写请求
 *
 * 工作原理：
 * 1. 初始化I2C模块，配置时钟和引脚
 * 2. 根据角色选择执行不同逻辑：
 *    - 主模式：主动发起读写操作
 *    - 从模式：通过中断响应主设备请求
 * 3. 支持EEPROM的读写操作，使用两字节地址访问
 *
 ****************************************************************************************
 */

#include "string.h"
#include "b6x.h"
#include "drvs.h"
#include "dbg.h"


/*
 * DEFINES
 ****************************************************************************************
 */

#define PIN_I2C_SCL            PA11  ///< I2C时钟线引脚
#define PIN_I2C_SDA            PA12  ///< I2C数据线引脚
#define GPIO_I2C_TEST          GPIO02

#define I2C_SLV_ADDR           0x57  ///< AT24C64 EEPROM设备地址
#define I2C_SLV_WRITE          ((I2C_SLV_ADDR << 1) | 0x0)  ///< 写操作地址
#define I2C_SLV_READ           ((I2C_SLV_ADDR << 1) | 0x1)  ///< 读操作地址

#define AT24C64_EEP            ((I2C_SLV_ADDR << 1))  ///< EEPROM设备地址

#define AT24C64_DATA_ADDR      0x20  ///< EEPROM数据存储地址

#define DATA_BUFF_LEN          16    ///< 数据缓冲区长度


/*
 * FUNCTIONS
 ****************************************************************************************
 */

#if (I2C_ROLE_SLAVE)

#define MEM_ADDR_BASE 0x20008000  ///< 从设备内存基地址

uint8_t dataBuff[258] = {0};     ///< 数据接收缓冲区
uint16_t buff_pos = 0;           ///< 缓冲区当前位置
uint32_t mem_addr = 0;           ///< 内存访问地址

/**
 ****************************************************************************************
 * @brief I2C中断服务函数（轮询方式）
 *
 * @details
 * 处理I2C从设备的各种状态：
 * - 接收写地址：准备接收数据
 * - 接收数据：存储到缓冲区
 * - 接收停止/重启信号：处理完整数据包
 * - 接收读地址/发送数据：响应主设备读请求
 ****************************************************************************************
 */
static void i2c_isr(void)
{
    if (I2C_IFLG_GET() == 0)
    {
        return;
    }
    
    debug("I2C status:0x%02X\r\n", I2C_STAT_GET());
    
    switch (I2C_STAT_GET())
    { 
        case I2C_SS_RECV_ADDR_W:
        {
            ///< 从设备接收到写地址，发送ACK
        } break;
        
        case I2C_SS_RECV_DAT_ACK:
        {
            ///< 从设备接收数据，发送ACK
            I2C_DATA_GET(dataBuff[buff_pos++]);  ///< 读取数据到缓冲区
        } break;
        
        case I2C_SS_RECV_STOP_OR_RESTART:
        {
            ///< 接收到停止或重启信号，处理完整数据包
            mem_addr = (MEM_ADDR_BASE + (dataBuff[0]<<8|dataBuff[1]));

            if (buff_pos > 2)
            {
                memcpy((uint8_t *)(mem_addr), &dataBuff[2], buff_pos-2);
            }
            buff_pos = 0;
        } break;
            
        case I2C_SS_RECV_ADDR_R_ACK:      
        case I2C_SS_SEND_DAT_ACK:
        {
            ///< 从设备发送数据，响应主设备读请求
            I2C_DATA_PUT(*(uint8_t *)(mem_addr++));  ///< 发送内存数据
        } break;
    }

    I2C_IFLG_CLR();  ///< 清除中断标志位
    
    NVIC_ClearPendingIRQ(I2C_IRQn);
    NVIC_EnableIRQ(I2C_IRQn);
}

/**
 ****************************************************************************************
 * @brief I2C硬件中断处理函数
 ****************************************************************************************
 */
void I2C_IRQHandler(void)
{
    NVIC_DisableIRQ(I2C_IRQn);
    
    //i2c_isr();  ///< 实际使用轮询方式，此处注释
}

/**
 ****************************************************************************************
 * @brief I2C从设备处理函数
 *
 * @details
 * 使用轮询方式持续检查I2C状态并处理
 ****************************************************************************************
 */
static void i2cProc(void)
{
    while (1)
    {
        i2c_isr(); // 轮询方式处理
    }
}

#else // (I2C_ROLE_MASTER)

/**
 ****************************************************************************************
 * @brief I2C主设备处理函数
 *
 * @details
 * 演示I2C主设备如何访问EEPROM：
 * 1. 通过直接I2C命令读写单个字节
 * 2. 使用EEPROM专用函数进行块读写
 * 3. 验证读写数据的正确性
 ****************************************************************************************
 */
static void i2cProc(void)
{
    uint16_t cnt;
    uint8_t buff_tx[DATA_BUFF_LEN];  ///< 发送数据缓冲区
    uint8_t buff_rx[DATA_BUFF_LEN];  ///< 接收数据缓冲区

    // I2C直接访问EEPROM测试
    debug("** I2C Master RD(0x%02X)/WR(0x%02X) 1B \r\n", I2C_SLV_READ, I2C_SLV_WRITE);
    
    buff_tx[0] = (AT24C64_DATA_ADDR >> 8) & 0xFF;  ///< EEPROM地址高字节
    buff_tx[1] = AT24C64_DATA_ADDR & 0xFF;         ///< EEPROM地址低字节
    buff_tx[2] = 0x5A;                             ///< 测试数据
    
    i2c_write(I2C_SLV_WRITE, 3, buff_tx);          ///< 写入1字节数据（3长度-2地址）
    debug("I2C WR_E2(ByteVal=0x5A)...\r\n");
    bootDelayMs(60);                               ///< 等待EEPROM写入完成
    
    i2c_write(I2C_SLV_WRITE, 2, buff_tx);          ///< 设置EEPROM地址
    cnt = i2c_read(I2C_SLV_READ, 1, buff_rx);      ///< 读取1字节数据
    debug("I2C RD_E2(len=%d, ByteVal=0x%02X) -> %s\r\n", cnt, buff_rx[0], (buff_rx[0]==0x5A)?"OK":"FAIL");
    
    // EEPROM块读写测试
    debug("** Eeprom(0x%02X) RD/WR at_0x%04X \r\n", AT24C64_EEP, AT24C64_DATA_ADDR);
    
    cnt = eeprom_read(AT24C64_EEP, AT24C64_DATA_ADDR, DATA_BUFF_LEN, buff_rx);
    debug("Read(len=%d) before:\r\n", cnt);
    debugHex(buff_rx, cnt);
    
    for (uint16_t i = 0; i < DATA_BUFF_LEN; i++)
    {
        buff_tx[i] = buff_rx[i] + 2;  ///< 生成测试数据：原数据+2
    }
    
    cnt = eeprom_write(AT24C64_EEP, AT24C64_DATA_ADDR, DATA_BUFF_LEN, buff_tx);
    debug("Write(len=%d), autoInc 2...\r\n", cnt);
    bootDelayMs(60);  ///< 等待EEPROM写入完成

    cnt = eeprom_read(AT24C64_EEP, AT24C64_DATA_ADDR, DATA_BUFF_LEN, buff_rx);
    debug("Read(len=%d) after:\r\n", cnt);
    debugHex(buff_rx, cnt);
    
    if ((cnt == DATA_BUFF_LEN) && (memcmp(buff_tx, buff_rx, DATA_BUFF_LEN) == 0))
    {
        debug("** I2C-Master Test End -> OK\r\n\r\n");
    }
    else
    {
        debug("** I2C-Master Test End -> FAIL\r\n\r\n");
    }
    
    while (1)
    {
        // 空循环
    };
}
#endif

/**
 ****************************************************************************************
 * @brief I2C初始化函数
 *
 * @details
 * 根据配置的角色初始化I2C模块：
 * - 配置引脚和时钟
 * - 从模式：设置从设备地址和中断
 * - 主模式：准备进行主动通信
 ****************************************************************************************
 */
static void i2cInit(void)
{
    i2c_init(PIN_I2C_SCL, PIN_I2C_SDA, SCLK_100K);  ///< 初始化I2C，100KHz时钟
    
    #if (I2C_ROLE_SLAVE)
    I2C_SET_ACK();        ///< 设置ACK响应
    I2C_SLV_ADR(0x57);    ///< 设置从设备地址

    // 使能中断
    I2C_INT_EN();         ///< 使能I2C中断
    
    NVIC_ClearPendingIRQ(I2C_IRQn);
    NVIC_EnableIRQ(I2C_IRQn);
    __enable_irq();
    #endif
}

/**
 ****************************************************************************************
 * @brief I2C测试主函数
 *
 * @details
 * 执行I2C模块的完整测试流程：
 * 1. 系统时钟配置（FPGA测试环境）
 * 2. I2C初始化
 * 3. 执行主/从设备测试逻辑
 ****************************************************************************************
 */
void i2cTest(void)
{
    #if (FPGA_TEST)
    // FPGA 48M 需要使用24M 20221119 --whl
    RCC->APB1CLK_DIV = 1;  ///< 设置APB1时钟分频
    #endif
    
    debug("i2cTest Start...\r\n"); 

    i2cInit();    ///< 初始化I2C模块

    i2cProc();    ///< 执行I2C处理逻辑
}
