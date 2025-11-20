/**
 ****************************************************************************************
 *
 * @file spi.c
 *
 * @brief Serial Peripheral Interface(SPI) Master/Slave Role Driver
 *
 ****************************************************************************************
 */

#include "spi.h"
#include "rcc.h"
#include "iopad.h"
#include "reg_spim.h"
#include "reg_spis.h"

/*
 * DEFINES
 ****************************************************************************************
 */

#define NSG_BYTE        (0xFF) ///< 非有效数据字节

/// SPI主设备开始传输宏
#define SPIM_BGN(len)   dowl( SPIM->DAT_LEN = (len); SPIM->TXRX_BGN = 1; )

/// SPI主设备等待传输完成宏
#define SPIM_WAIT()     dowl( while (SPIM->STATUS.SPIM_BUSY); SPIM->STATUS_CLR.Word = SPIM_STATUS_CLR_ALL; )

/*
 * FUNCTIONS - SPI Master
 ****************************************************************************************
 */

/**
 * @brief 初始化SPI主设备
 * @param io_clk  时钟引脚
 * @param io_miso 主输入从输出引脚
 * @param io_mosi 主输出从输入引脚
 */
void spim_init(uint8_t io_clk, uint8_t io_miso, uint8_t io_mosi)
{
    // i2c_clk_en rst_req or soft_reset
    RCC_AHBCLK_EN(AHB_SPIM_BIT);  /* 使能SPI主设备时钟 */
    RCC_AHBRST_REQ(AHB_SPIM_BIT); /* 请求SPI主设备复位 */

    // csc connect
    csc_output(io_clk,  CSC_SPIM_CLK);  /* 配置时钟引脚为SPI主设备时钟输出 */
    csc_input(io_miso,  CSC_SPIM_MISO); /* 配置MISO引脚为SPI主设备数据输入 */
    csc_output(io_mosi, CSC_SPIM_MOSI); /* 配置MOSI引脚为SPI主设备数据输出 */

    // iomode control
    iom_ctrl(io_clk,  IOM_SEL_CSC | IOM_DRV_LVL1 | IOM_PULLUP); /* 时钟引脚：CSC功能、驱动电平1、上拉 */
    iom_ctrl(io_miso, IOM_SEL_CSC | IOM_PULLUP   | IOM_INPUT);  /* MISO引脚：CSC功能、上拉、输入模式 */
    iom_ctrl(io_mosi, IOM_SEL_CSC | IOM_DRV_LVL1 | IOM_PULLUP); /* MOSI引脚：CSC功能、驱动电平1、上拉 */
}

/**
 * @brief 配置SPI主设备控制参数
 * @param ctrl 控制寄存器配置值
 */
void spim_conf(uint32_t ctrl)
{
    // clear FiFo and inner counter
    SPIM->STATUS_CLR.Word = SPIM_STATUS_CLR_ALL; /* STATUS_CLR: 清除所有状态标志 */

    // config ctrl
    SPIM->CTRL.Word = ctrl; /* CTRL: 配置SPI主设备控制参数 */
}

/**
 * @brief 开始SPI传输
 * @param data_len 数据传输长度
 */
void spim_begin(uint16_t data_len)
{
    //if (SPIM->STATUS.SPIM_BUSY == 0) - rm, dma mode not fit
    SPIM_BGN(data_len); /* DAT_LEN: 设置数据长度，TXRX_BGN: 开始传输 */
}

/**
 * @brief 等待SPI传输完成
 */
void spim_wait(void)
{
    SPIM_WAIT(); /* STATUS: 等待SPI忙标志清除，STATUS_CLR: 清除所有状态标志 */
}

/**
 * @brief SPI全双工数据传输
 * @param tx_data 发送数据缓冲区指针
 * @param rx_buff 接收数据缓冲区指针
 * @param length  数据传输长度
 */
void spim_duplex(const uint8_t *tx_data, uint8_t *rx_buff, uint16_t length)
{
    uint32_t tidx, ridx;

    SPIM_BGN(length); /* DAT_LEN: 设置数据长度，TXRX_BGN: 开始传输 */

    for (tidx = 0, ridx = 0; ridx < length; )
    {
        if ((tidx < length) && (SPIM->STATUS.SPIM_TX_FFULL == 0)) /* STATUS: 检查发送FIFO未满 */
        {
            SPIM->TX_DATA = tx_data[tidx++]; /* TX_DATA: 写入发送数据 */
        }

        if (SPIM->STATUS.SPIM_RX_FEMPTY == 0) /* STATUS: 检查接收FIFO非空 */
        {
            rx_buff[ridx++] = SPIM->RX_DATA; /* RX_DATA: 读取接收数据 */
        }
    }

    SPIM_WAIT(); /* STATUS: 等待SPI忙标志清除，STATUS_CLR: 清除所有状态标志 */
}

/**
 * @brief SPI发送数据
 * @param tx_data 发送数据缓冲区指针
 * @param length  发送数据长度
 */
void spim_transimit(const uint8_t *tx_data, uint16_t length)
{
    uint32_t tidx, ridx;;

    SPIM_BGN(length); /* DAT_LEN: 设置数据长度，TXRX_BGN: 开始传输 */

    // send cmd
    for (tidx = 0, ridx = 0; ridx < length; )
    {
        if ((tidx < length) && (SPIM->STATUS.SPIM_TX_FFULL == 0)) /* STATUS: 检查发送FIFO未满 */
        {
            SPIM->TX_DATA = tx_data[tidx++]; /* TX_DATA: 写入发送数据 */
        }

        if (SPIM->STATUS.SPIM_RX_FEMPTY == 0) /* STATUS: 检查接收FIFO非空 */
        {
            SPIM->RX_DATA; // drop /* RX_DATA: 丢弃接收数据 */
            ridx++;
        }
    }

    SPIM_WAIT(); /* STATUS: 等待SPI忙标志清除，STATUS_CLR: 清除所有状态标志 */
}

/**
 * @brief SPI接收数据
 * @param rx_buff 接收数据缓冲区指针
 * @param length  接收数据长度
 */
void spim_receive(uint8_t *rx_buff, uint16_t length)
{
    uint32_t tidx, ridx;

    SPIM_BGN(length); /* DAT_LEN: 设置数据长度，TXRX_BGN: 开始传输 */

    // recv rsp
    for (tidx = 0, ridx = 0; ridx < length; )
    {
        if ((tidx < length) && (SPIM->STATUS.SPIM_TX_FFULL == 0)) /* STATUS: 检查发送FIFO未满 */
        {
            SPIM->TX_DATA = NSG_BYTE; /* TX_DATA: 发送非有效数据以产生时钟 */
            tidx++;
        }

        if (SPIM->STATUS.SPIM_RX_FEMPTY == 0) /* STATUS: 检查接收FIFO非空 */
        {
            rx_buff[ridx++] = SPIM->RX_DATA; /* RX_DATA: 读取接收数据 */
        }
    }

    SPIM_WAIT(); /* STATUS: 等待SPI忙标志清除，STATUS_CLR: 清除所有状态标志 */
}

/**
 * @brief SPI半双工数据传输（先发送后接收）
 * @param cmd  命令数据缓冲区指针
 * @param clen 命令数据长度
 * @param rsp  响应数据缓冲区指针
 * @param rlen 响应数据长度
 */
void spim_halfdx(const uint8_t *cmd, uint16_t clen, uint8_t *rsp, uint16_t rlen)
{
    uint32_t tidx, ridx;

    SPIM_BGN(clen + rlen); /* DAT_LEN: 设置总数据长度，TXRX_BGN: 开始传输 */

    // send cmd
    for (tidx = 0, ridx = 0; ridx < clen; )
    {
        if ((tidx < clen) && (SPIM->STATUS.SPIM_TX_FFULL == 0)) /* STATUS: 检查发送FIFO未满 */
        {
            SPIM->TX_DATA = cmd[tidx++]; /* TX_DATA: 写入命令数据 */
        }

        if (SPIM->STATUS.SPIM_RX_FEMPTY == 0) /* STATUS: 检查接收FIFO非空 */
        {
            SPIM->RX_DATA; // drop /* RX_DATA: 丢弃接收数据 */
            ridx++;
        }
    }

    // recv rsp
    for (tidx = 0, ridx = 0; ridx < rlen; )
    {
        if ((tidx < rlen) && (SPIM->STATUS.SPIM_TX_FFULL == 0)) /* STATUS: 检查发送FIFO未满 */
        {
            SPIM->TX_DATA = NSG_BYTE; /* TX_DATA: 发送非有效数据以产生时钟 */
            tidx++;
        }

        if (SPIM->STATUS.SPIM_RX_FEMPTY == 0) /* STATUS: 检查接收FIFO非空 */
        {
            rsp[ridx++] = SPIM->RX_DATA; /* RX_DATA: 读取响应数据 */
        }
    }

    SPIM_WAIT(); /* STATUS: 等待SPI忙标志清除，STATUS_CLR: 清除所有状态标志 */
}

/*
 * FUNCTIONS - SPI Slave
 ****************************************************************************************
 */

/**
 * @brief 初始化SPI从设备
 * @param io_cs   片选引脚
 * @param io_clk  时钟引脚
 * @param io_miso 主输入从输出引脚
 * @param io_mosi 主输出从输入引脚
 */
void spis_init(uint8_t io_cs, uint8_t io_clk, uint8_t io_miso, uint8_t io_mosi)
{
    // i2c_clk_en rst_req or soft_reset
    RCC_AHBCLK_EN(AHB_SPIS_BIT);  /* 使能SPI从设备时钟 */
    RCC_AHBRST_REQ(AHB_SPIS_BIT); /* 请求SPI从设备复位 */

    // csc connect
    csc_input(io_cs,    CSC_SPIS_CSN);  /* 配置片选引脚为SPI从设备片选输入 */
    csc_input(io_clk,   CSC_SPIS_CLK);  /* 配置时钟引脚为SPI从设备时钟输入 */
    csc_output(io_miso, CSC_SPIS_MISO); /* 配置MISO引脚为SPI从设备数据输出 */
    csc_input(io_mosi,  CSC_SPIS_MOSI); /* 配置MOSI引脚为SPI从设备数据输入 */

    // iomode control
    iom_ctrl(io_cs,   IOM_SEL_CSC | IOM_PULLUP   | IOM_INPUT);  /* 片选引脚：CSC功能、上拉、输入模式 */
    iom_ctrl(io_clk,  IOM_SEL_CSC | IOM_PULLUP   | IOM_INPUT);  /* 时钟引脚：CSC功能、上拉、输入模式 */
    iom_ctrl(io_miso, IOM_SEL_CSC | IOM_DRV_LVL1 | IOM_PULLUP); /* MISO引脚：CSC功能、驱动电平1、上拉 */
    iom_ctrl(io_mosi, IOM_SEL_CSC | IOM_PULLUP   | IOM_INPUT);  /* MOSI引脚：CSC功能、上拉、输入模式 */
}

/**
 * @brief 配置SPI从设备控制参数
 * @param ctrl 控制寄存器配置值
 */
void spis_conf(uint32_t ctrl)
{
    // disable run
    SPIS->CTRL.Word = 0; /* CTRL: 禁用SPI从设备 */
    // clear FiFo and Interrupt
    SPIS->INFO_CLR.Word = SPIS_INFO_CLR_ALL; /* INFO_CLR: 清除所有信息和中断标志 */

    // config ctrl
    SPIS->CTRL.Word = ctrl; /* CTRL: 配置SPI从设备控制参数 */
}

/**
 * @brief 从SPI从设备接收一个字节
 * @param ch 接收数据指针
 * @return true - 成功接收，false - 接收FIFO为空
 */
bool spis_getc(uint8_t *ch)
{
    if (SPIS->STATUS.SPIS_RXFIFO_EMPTY == 0) /* STATUS: 检查接收FIFO非空 */
    {
        *ch = SPIS->RX_DAT; /* RX_DAT: 读取接收数据 */
        //SPIS->INFO_CLR.Word = SPIS_RXINT_CLR_BIT;
        return true;
    }

    return false;
}

/**
 * @brief 向SPI从设备发送一个字节
 * @param ch 要发送的字节
 */
void spis_putc(uint8_t ch)
{
    while (SPIS->STATUS.SPIS_TXFIFO_FULL); /* STATUS: 等待发送FIFO非满 */
    SPIS->TX_DAT = ch; /* TX_DAT: 写入发送数据 */
}
