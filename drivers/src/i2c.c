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


/*
 * DEFINES
 ****************************************************************************************
 */

// Must clear SI before setting STA and then STA must be manually cleared.
#define I2C_START()                                         \
    dowl(                                                   \
        I2C->CONCLR.Word = I2C_CR_IFLG_BIT;                 \
        I2C->CONSET.Word = I2C_CR_START_BIT;                \
        while ((I2C->CONSET.Word & I2C_CR_IFLG_BIT) == 0);  \
        I2C->CONCLR.Word = I2C_CR_START_BIT;                \
    )

// Must set STO before clearing SI.
#define I2C_STOP()                                          \
    dowl(                                                   \
        I2C->CONSET.Word = I2C_CR_STOP_BIT;                 \
        I2C->CONCLR.Word = I2C_CR_IFLG_BIT;                 \
        while ((I2C->CONSET.Word & I2C_CR_STOP_BIT) != 0);  \
    )

// Stop clock-stretching and then read when it arrives.
#define I2C_GETC(_X_)                                       \
    dowl(                                                   \
        I2C->CONCLR.Word = I2C_CR_IFLG_BIT;                 \
        while ((I2C->CONSET.Word & I2C_CR_IFLG_BIT) == 0);  \
        (_X_) = I2C->DATA.Word;                             \
    )

// First write new data and then stop clock-stretching.
#define I2C_PUTC(_X_)                                       \
    dowl(                                                   \
        I2C->DATA.Word = (_X_);                             \
        I2C->CONCLR.Word = I2C_CR_IFLG_BIT;                 \
        while ((I2C->CONSET.Word & I2C_CR_IFLG_BIT) == 0);  \
    )


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void i2c_init(uint8_t io_scl, uint8_t io_sda, uint8_t sclk)
{
    // i2c_clk_en rst_req or soft_reset
    RCC_APBCLK_EN(APB_I2C_BIT);
    //RCC_APBRST_REQ(APB_I2C_BIT);
    I2C->SRST = 0x07;

    // iocsc_i2c(io_scl, io_sda);
    csc_output(io_scl, CSC_I2C_SCL);
    csc_input(io_scl,  CSC_I2C_SCL);
    csc_output(io_sda, CSC_I2C_SDA);
    csc_input(io_sda,  CSC_I2C_SDA);

    iom_ctrl(io_scl, IOM_SEL_CSC | IOM_DRV_LVL1 | IOM_PULLUP | IOM_INPUT | IOM_OPENDRAIN);
    iom_ctrl(io_sda, IOM_SEL_CSC | IOM_DRV_LVL1 | IOM_PULLUP | IOM_INPUT | IOM_OPENDRAIN);

    // SCL clock = PCLK/(2^clk_samp * (clk_scl+1) *10)
    I2C->CLK.Word = sclk;
    I2C_ENABLE();
}

void i2c_deinit(void)
{
    I2C_DISABLE();
    RCC_APBCLK_DIS(1 << (RCC_I2C_CLKEN_RUN_POS));
}

uint8_t i2c_start(uint8_t slv_addr)
{
    I2C_START();

    // A start condition has been transmitted
    if ((I2C_STAT_GET() == I2C_MS_START_OK) || (I2C_STAT_GET() == I2C_MS_RESTART_OK))
    {
        I2C_PUTC(slv_addr);
    }

    return I2C_STAT_GET();
}

void i2c_stop(void)
{
    I2C_STOP();
}

uint16_t i2c_send(uint16_t len, const uint8_t *data)
{
    uint16_t cnt;

    for (cnt = 0; cnt < len; cnt++)
    {
        I2C_PUTC(*data++);

        if (I2C_STAT_GET() != I2C_MS_SEND_DAT_ACK)
        {
            if (I2C_STAT_GET() == I2C_MS_SEND_DAT_NAK)
            {
                cnt += 1; // last byte
            }
            break;
        }
    }

    return cnt;
}

uint16_t i2c_recv(uint16_t len, uint8_t *buff)
{
    uint16_t cnt;

    // All bytes are ACK'd except for the last one which is NACK'd. If only
    // 1 byte is being read, a single NACK will be sent. Thus, we only want
    // to enable ACK if more than 1 byte is going to be read.
    if (len > 1)
    {
        I2C_SET_ACK();
    }

    for (cnt = 0; cnt < len; cnt++)
    {
        // slave devices require NACK to be sent after reading last byte
        if (cnt + 1 == len)
        {
            I2C_SET_NAK();
        }

        // read a byte from the I2C interface
        I2C_GETC(*buff++);

        if (I2C_STAT_GET() != I2C_MS_READ_DAT_ACK)
        {
            if (I2C_STAT_GET() == I2C_MS_READ_DAT_NAK)
            {
                cnt += 1; // count last byte
            }
            break;
        }
    }

    return cnt;
}

uint16_t i2c_read(uint8_t addr_r, uint16_t len, uint8_t *buff)
{
    uint16_t cnt = 0;

    if (i2c_start(addr_r) == I2C_MS_ADDR_R_ACK)
    {
        cnt = i2c_recv(len, buff);
    }

    i2c_stop();
    return cnt;
}

uint16_t i2c_write(uint8_t addr_w, uint16_t len, const uint8_t *data)
{
    uint16_t cnt = 0;

    if (i2c_start(addr_w) == I2C_MS_ADDR_W_ACK)
    {
        cnt = i2c_send(len, data);
    }

    i2c_stop();
    return cnt;
}

uint16_t eeprom_write(uint8_t eep, uint16_t addr, uint16_t len, const uint8_t *data)
{
    uint16_t cnt = 0;

    if (i2c_start(eep) == I2C_MS_ADDR_W_ACK)
    {
        #if (OPTM_FAST)
        do {
            I2C_PUTC((addr >> 8) & 0xff); // address High byte
            if (I2C_STAT_GET() != I2C_MS_SEND_DAT_ACK) break;

            I2C_PUTC(addr & 0xff); // address High byte
            if (I2C_STAT_GET() != I2C_MS_SEND_DAT_ACK) break;

            for (; cnt < len; cnt++)
            {
                I2C_PUTC(*data++);

                if (I2C_STAT_GET() != I2C_MS_SEND_DAT_ACK)
                {
                    if (I2C_STAT_GET() == I2C_MS_SEND_DAT_NAK)
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

    i2c_stop();
    return cnt;
}

uint16_t eeprom_read(uint8_t eep, uint16_t addr, uint16_t len, uint8_t *buff)
{
    uint16_t cnt = 0;

    if (i2c_start(eep) == I2C_MS_ADDR_W_ACK)
    {
        #if (OPTM_FAST)
        do {
            I2C_PUTC((addr >> 8) & 0xff); // address High byte
            if (I2C_STAT_GET() != I2C_MS_SEND_DAT_ACK) break;

            I2C_PUTC(addr & 0xff); // address High byte
            if (I2C_STAT_GET() != I2C_MS_SEND_DAT_ACK) break;

            if (i2c_start(eep | I2C_RD_OP) != I2C_MS_ADDR_R_ACK) break;

            if (len > 1)
            {
                I2C_SET_ACK();
            }

            for (; cnt < len; cnt++)
            {
                // slave devices require NACK to be sent after reading last byte
                if (cnt + 1 == len)
                {
                    I2C_SET_NAK();
                }

                // read a byte from the I2C interface
                I2C_GETC(buff[cnt]);

                if (I2C_STAT_GET() != I2C_MS_READ_DAT_ACK)
                {
                    if (I2C_STAT_GET() == I2C_MS_READ_DAT_NAK)
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

    i2c_stop();
    return cnt;
}
