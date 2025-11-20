/**
 ****************************************************************************************
 *
 * @file i2c.c
 *
 * @brief IIC(I2C) Driver
 *
 ****************************************************************************************
 */

#include "i2c.h"
#include "rcc.h"
#include "iopad.h"
#include "reg_i2c.h"
#include "reg_gpio.h"
#include "core_cmInstr.h"

/*
 * DEFINES
 ****************************************************************************************
 */

// 必须先清除SI标志再设置STA，然后必须手动清除STA
#define I2C_START()                                         \
    dowl(                                                   \
        I2C->CONCLR.Word = I2C_CR_IFLG_BIT;                 /* CONCLR: 清除中断标志(IFLG) */ \
        I2C->CONSET.Word = I2C_CR_START_BIT;                /* CONSET: 设置起始条件(STA) */ \
        while ((I2C->CONSET.Word & I2C_CR_IFLG_BIT) == 0);  /* 等待中断标志置位 */ \
        I2C->CONCLR.Word = I2C_CR_START_BIT;                /* CONCLR: 清除起始条件(STA) */ \
    )

// 必须先设置STO再清除SI
#define I2C_STOP()                                          \
    dowl(                                                   \
        I2C->CONSET.Word = I2C_CR_STOP_BIT;                 /* CONSET: 设置停止条件(STO) */ \
        I2C->CONCLR.Word = I2C_CR_IFLG_BIT;                 /* CONCLR: 清除中断标志(IFLG) */ \
        while ((I2C->CONSET.Word & I2C_CR_STOP_BIT) != 0);  /* 等待停止条件完成 */ \
    )

// 停止时钟拉伸然后读取数据
#define I2C_GETC(_X_)                                       \
    dowl(                                                   \
        I2C->CONCLR.Word = I2C_CR_IFLG_BIT;                 /* CONCLR: 清除中断标志(IFLG) */ \
        while ((I2C->CONSET.Word & I2C_CR_IFLG_BIT) == 0);  /* 等待中断标志置位 */ \
        (_X_) = I2C->DATA.Word;                             /* DATA: 读取数据 */ \
    )

// 先写入新数据然后停止时钟拉伸
#define I2C_PUTC(_X_)                                       \
    dowl(                                                   \
        I2C->DATA.Word = (_X_);                             /* DATA: 写入数据 */ \
        I2C->CONCLR.Word = I2C_CR_IFLG_BIT;                 /* CONCLR: 清除中断标志(IFLG) */ \
        while ((I2C->CONSET.Word & I2C_CR_IFLG_BIT) == 0);  /* 等待中断标志置位 */ \
    )

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 * @brief 初始化I2C接口
 * @param io_scl SCL引脚
 * @param io_sda SDA引脚
 * @param sclk 时钟配置值
 */
void i2c_init(uint8_t io_scl, uint8_t io_sda, uint8_t sclk)
{
    // i2c_clk_en rst_req or soft_reset
    RCC_APBCLK_EN(APB_I2C_BIT);                            /* 使能I2C时钟 */
    //RCC_APBRST_REQ(APB_I2C_BIT);
    I2C->SRST = 0x07;                                      /* SRST: 软件复位I2C */

    uint32_t io_ctrl = IOM_SEL_CSC | IOM_DRV_LVL1 | IOM_PULLUP | IOM_INPUT | IOM_OPENDRAIN;

    iom_ctrl(io_sda, IOM_SEL_GPIO | IOM_PULLDOWN | IOM_INPUT);  /* 配置SDA引脚为上拉输入 */

    /**********************************/
    // 2024.10.23 --- wq.
    uint16_t delay_cnt = 750;
    // Delay for io cfg stable. 64M about 180us.
    while (delay_cnt--)
    {
        __NOP();__NOP();
        __NOP();__NOP();
        __NOP();__NOP();
        __NOP();__NOP();
    }
    /**********************************/

    // if Extern Pull-Up, disable Internal Pull-Up
    if ((GPIO->PIN >> io_sda) & 0x01)                      /* 检查外部上拉 */
    {
        io_ctrl = IOM_SEL_CSC | IOM_DRV_LVL1 | IOM_INPUT | IOM_OPENDRAIN;
    }

    // iocsc_i2c(io_scl, io_sda);
    csc_output(io_scl, CSC_I2C_SCL);                       /* 配置SCL引脚为I2C时钟输出 */
    csc_input(io_scl,  CSC_I2C_SCL);                       /* 配置SCL引脚为I2C时钟输入 */
    csc_output(io_sda, CSC_I2C_SDA);                       /* 配置SDA引脚为I2C数据输出 */
    csc_input(io_sda,  CSC_I2C_SDA);                       /* 配置SDA引脚为I2C数据输入 */

    iom_ctrl(io_scl, io_ctrl);                             /* 配置SCL引脚IO模式 */
    iom_ctrl(io_sda, io_ctrl);                             /* 配置SDA引脚IO模式 */

    // SCL clock = PCLK/(2^clk_samp * (clk_scl+1) *10)
    I2C->CLK.Word = sclk;                                  /* CLK: 设置I2C时钟分频 */
    I2C_ENABLE();                                          /* CONSET: 使能I2C(I2CEN) */
}

/**
 * @brief 反初始化I2C接口
 */
void i2c_deinit(void)
{
    RCC_APBCLK_DIS(APB_I2C_BIT);                           /* 禁用I2C时钟 */
    RCC_APBRST_REQ(APB_I2C_BIT);                           /* 请求I2C复位 */
}

/**
 * @brief 发送I2C起始条件
 * @param slv_addr 从设备地址
 * @return I2C状态码
 */
uint8_t i2c_start(uint8_t slv_addr)
{
    I2C_START();                                           /* 发送起始条件 */

    // A start condition has been transmitted
    if ((I2C_STAT_GET() == I2C_MS_START_OK) || (I2C_STAT_GET() == I2C_MS_RESTART_OK))
    {
        I2C_PUTC(slv_addr);                                /* DATA: 发送从设备地址 */
    }

    return I2C_STAT_GET();                                 /* SR: 返回状态寄存器值 */
}

/**
 * @brief 发送I2C停止条件
 */
void i2c_stop(void)
{
    I2C_STOP();                                            /* 发送停止条件 */
}

/**
 * @brief I2C发送数据
 * @param len 数据长度
 * @param data 数据指针
 * @return 实际发送的字节数
 */
uint16_t i2c_send(uint16_t len, const uint8_t *data)
{
    uint16_t cnt;

    for (cnt = 0; cnt < len; cnt++)
    {
        I2C_PUTC(*data++);                                 /* DATA: 发送数据字节 */

        if (I2C_STAT_GET() != I2C_MS_SEND_DAT_ACK)         /* SR: 检查ACK响应 */
        {
            if (I2C_STAT_GET() == I2C_MS_SEND_DAT_NAK)     /* SR: 检查NAK响应 */
            {
                cnt += 1; // last byte
            }
            break;
        }
    }

    return cnt;
}

/**
 * @brief I2C接收数据
 * @param len 数据长度
 * @param buff 接收缓冲区指针
 * @return 实际接收的字节数
 */
uint16_t i2c_recv(uint16_t len, uint8_t *buff)
{
    uint16_t cnt;

    // All bytes are ACK'd except for the last one which is NACK'd. If only
    // 1 byte is being read, a single NACK will be sent. Thus, we only want
    // to enable ACK if more than 1 byte is going to be read.
    if (len > 1)
    {
        I2C_SET_ACK();                                     /* CONSET: 设置应答位(AAK) */
    }

    for (cnt = 0; cnt < len; cnt++)
    {
        // slave devices require NACK to be sent after reading last byte
        if (cnt + 1 == len)
        {
            I2C_SET_NAK();                                 /* CONCLR: 清除应答位(AAK) */
        }

        // read a byte from the I2C interface
        I2C_GETC(*buff++);                                 /* DATA: 读取数据字节 */

        if (I2C_STAT_GET() != I2C_MS_READ_DAT_ACK)         /* SR: 检查ACK响应 */
        {
            if (I2C_STAT_GET() == I2C_MS_READ_DAT_NAK)     /* SR: 检查NAK响应 */
            {
                cnt += 1; // count last byte
            }
            break;
        }
    }

    return cnt;
}

/**
 * @brief I2C读取操作
 * @param addr_r 读地址
 * @param len 数据长度
 * @param buff 接收缓冲区指针
 * @return 实际读取的字节数
 */
uint16_t i2c_read(uint8_t addr_r, uint16_t len, uint8_t *buff)
{
    uint16_t cnt = 0;

    if (i2c_start(addr_r) == I2C_MS_ADDR_R_ACK)            /* 发送读地址并检查ACK */
    {
        cnt = i2c_recv(len, buff);                         /* 接收数据 */
    }

    i2c_stop();                                            /* 发送停止条件 */
    return cnt;
}

/**
 * @brief I2C写入操作
 * @param addr_w 写地址
 * @param len 数据长度
 * @param data 数据指针
 * @return 实际写入的字节数
 */
uint16_t i2c_write(uint8_t addr_w, uint16_t len, const uint8_t *data)
{
    uint16_t cnt = 0;

    if (i2c_start(addr_w) == I2C_MS_ADDR_W_ACK)            /* 发送写地址并检查ACK */
    {
        cnt = i2c_send(len, data);                         /* 发送数据 */
    }

    i2c_stop();                                            /* 发送停止条件 */
    return cnt;
}

/**
 * @brief EEPROM写入操作
 * @param eep EEPROM设备地址
 * @param addr EEPROM内部地址
 * @param len 数据长度
 * @param data 数据指针
 * @return 实际写入的字节数
 */
uint16_t eeprom_write(uint8_t eep, uint16_t addr, uint16_t len, const uint8_t *data)
{
    uint16_t cnt = 0;

    if (i2c_start(eep) == I2C_MS_ADDR_W_ACK)               /* 发送EEPROM地址并检查ACK */
    {
        #if (OPTM_FAST)
        do {
            I2C_PUTC((addr >> 8) & 0xff);                  /* DATA: 发送地址高字节 */
            if (I2C_STAT_GET() != I2C_MS_SEND_DAT_ACK) break;

            I2C_PUTC(addr & 0xff);                         /* DATA: 发送地址低字节 */
            if (I2C_STAT_GET() != I2C_MS_SEND_DAT_ACK) break;

            for (; cnt < len; cnt++)
            {
                I2C_PUTC(*data++);                         /* DATA: 发送数据字节 */

                if (I2C_STAT_GET() != I2C_MS_SEND_DAT_ACK) /* SR: 检查ACK响应 */
                {
                    if (I2C_STAT_GET() == I2C_MS_SEND_DAT_NAK) /* SR: 检查NAK响应 */
                    {
                        cnt += 1; // last byte
                    }
                    break;
                }
            }
        } while (0);
        #else
        addr = (addr >> 8) | (addr << 8); // send address in MSB
        if (i2c_send(2, (uint8_t *)&addr) == 2)
        {
            cnt = i2c_send(len, data);
        }
        #endif
    }

    i2c_stop();                                            /* 发送停止条件 */
    return cnt;
}

/**
 * @brief EEPROM读取操作
 * @param eep EEPROM设备地址
 * @param addr EEPROM内部地址
 * @param len 数据长度
 * @param buff 接收缓冲区指针
 * @return 实际读取的字节数
 */
uint16_t eeprom_read(uint8_t eep, uint16_t addr, uint16_t len, uint8_t *buff)
{
    uint16_t cnt = 0;

    if (i2c_start(eep) == I2C_MS_ADDR_W_ACK)               /* 发送EEPROM地址并检查ACK */
    {
        #if (OPTM_FAST)
        do {
            I2C_PUTC((addr >> 8) & 0xff);                  /* DATA: 发送地址高字节 */
            if (I2C_STAT_GET() != I2C_MS_SEND_DAT_ACK) break;

            I2C_PUTC(addr & 0xff);                         /* DATA: 发送地址低字节 */
            if (I2C_STAT_GET() != I2C_MS_SEND_DAT_ACK) break;

            if (i2c_start(eep | I2C_RD_OP) != I2C_MS_ADDR_R_ACK) break; /* 发送读操作 */

            if (len > 1)
            {
                I2C_SET_ACK();                             /* CONSET: 设置应答位(AAK) */
            }

            for (; cnt < len; cnt++)
            {
                // slave devices require NACK to be sent after reading last byte
                if (cnt + 1 == len)
                {
                    I2C_SET_NAK();                         /* CONCLR: 清除应答位(AAK) */
                }

                // read a byte from the I2C interface
                I2C_GETC(buff[cnt]);                       /* DATA: 读取数据字节 */

                if (I2C_STAT_GET() != I2C_MS_READ_DAT_ACK) /* SR: 检查ACK响应 */
                {
                    if (I2C_STAT_GET() == I2C_MS_READ_DAT_NAK) /* SR: 检查NAK响应 */
                    {
                        cnt += 1; // count last byte
                    }
                    break;
                }
            }
        } while (0);
        #else
        addr = (addr >> 8) | (addr << 8); // send address in MSB
        if (i2c_send(2, (uint8_t *)&addr) == 2)
        {
            if (i2c_start(eep | I2C_RD_OP) == I2C_MS_ADDR_R_ACK)
            {
                cnt = i2c_recv(len, buff);
            }
        }
        #endif
    }

    i2c_stop();                                            /* 发送停止条件 */
    return cnt;
}
