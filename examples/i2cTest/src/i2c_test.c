/**
 ****************************************************************************************
 *
 * @file i2c_test.c
 *
 * @brief I2C master/slave device test
 *
 * @details
 * Demonstrates I2C master and slave modes (selected via I2C_ROLE_SLAVE in cfg.h):
 * - Master mode: read/write AT24C64 EEPROM using direct I2C and EEPROM helpers
 * - Slave mode: poll-based I2C slave emulating EEPROM behavior
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

#define PIN_I2C_SCL            PA11  ///< I2C clock pin
#define PIN_I2C_SDA            PA12  ///< I2C data pin

#define I2C_SLV_ADDR           0x57  ///< AT24C64 EEPROM device address
#define I2C_SLV_WRITE          ((I2C_SLV_ADDR << 1) | 0x0)
#define I2C_SLV_READ           ((I2C_SLV_ADDR << 1) | 0x1)

#define AT24C64_DATA_ADDR      0x20  ///< EEPROM data address for test
#define DATA_BUFF_LEN          16    ///< Data buffer length
#define EEPROM_WRITE_DELAY_MS  10    ///< EEPROM write cycle wait (AT24C64 tWR max 5ms)


/*
 * FUNCTIONS
 ****************************************************************************************
 */

#if (I2C_ROLE_SLAVE)

#define MEM_ADDR_BASE          0x20008000  ///< Slave memory base address
#define MEM_ADDR_END           0x20009000  ///< Slave memory end address (4KB window)

#define DATA_BUFF_SIZE         258         ///< Slave receive buffer (2-byte addr + 256-byte data)

static uint8_t  data_buff[DATA_BUFF_SIZE] = {0};
static uint16_t buff_pos = 0;
static uint32_t mem_addr = 0;

/**
 ****************************************************************************************
 * @brief I2C slave state handler (poll-based)
 *
 * @details
 * Handles I2C slave protocol states:
 * - Write address: prepare to receive
 * - Data receive: store to buffer
 * - Stop/restart: process complete packet
 * - Read address/data: respond to master read
 ****************************************************************************************
 */
static void i2c_slave_poll(void)
{
    if (I2C_IFLG_GET() == 0)
    {
        return;
    }

    switch (I2C_STAT_GET())
    {
        case I2C_SS_RECV_ADDR_W:
        {
            // Slave received write address, send ACK
            buff_pos = 0;
        } break;

        case I2C_SS_RECV_DAT_ACK:
        {
            // Slave received data, send ACK
            if (buff_pos < DATA_BUFF_SIZE)
            {
                I2C_DATA_GET(data_buff[buff_pos++]);
            }
        } break;

        case I2C_SS_RECV_STOP_OR_RESTART:
        {
            // Stop or restart received, process packet
            mem_addr = (MEM_ADDR_BASE + (data_buff[0] << 8 | data_buff[1]));

            if (buff_pos > 2)
            {
                uint16_t wr_len = buff_pos - 2;
                if ((mem_addr >= MEM_ADDR_BASE) && ((mem_addr + wr_len) <= MEM_ADDR_END))
                {
                    memcpy((uint8_t *)(uintptr_t)mem_addr, &data_buff[2], wr_len);
                }
            }
            buff_pos = 0;
        } break;

        case I2C_SS_RECV_ADDR_R_ACK:
        case I2C_SS_SEND_DAT_ACK:
        {
            // Slave send data, respond to master read
            if ((mem_addr >= MEM_ADDR_BASE) && (mem_addr < MEM_ADDR_END))
            {
                I2C_DATA_PUT(*(uint8_t *)(mem_addr));
            }
            else
            {
                I2C_DATA_PUT(0xFF);
            }
            mem_addr++;
        } break;
    }

    I2C_IFLG_CLR();
}

/**
 ****************************************************************************************
 * @brief I2C slave processing loop
 ****************************************************************************************
 */
static void i2c_proc(void)
{
    while (1)
    {
        i2c_slave_poll();
    }
}

#else // (I2C_ROLE_MASTER)

/**
 ****************************************************************************************
 * @brief I2C master test
 *
 * @details
 * Two-stage EEPROM test:
 * 1. Direct I2C API: single byte write/read (raw i2c_write/i2c_read)
 * 2. EEPROM helper API: block write/read (eeprom_write/eeprom_read)
 * Both stages verify data integrity.
 ****************************************************************************************
 */
static void i2c_proc(void)
{
    uint16_t cnt;
    uint8_t buff_tx[DATA_BUFF_LEN];
    uint8_t buff_rx[DATA_BUFF_LEN];

    // Stage 1: Direct I2C byte access
    debug("** I2C Master RD(0x%02X)/WR(0x%02X) 1B \r\n", I2C_SLV_READ, I2C_SLV_WRITE);

    buff_tx[0] = (AT24C64_DATA_ADDR >> 8) & 0xFF;
    buff_tx[1] = AT24C64_DATA_ADDR & 0xFF;
    buff_tx[2] = 0x5A;

    i2c_write(I2C_SLV_WRITE, 3, buff_tx);
    debug("I2C WR_E2(ByteVal=0x5A)...\r\n");
    bootDelayMs(EEPROM_WRITE_DELAY_MS);

    i2c_write(I2C_SLV_WRITE, 2, buff_tx);
    cnt = i2c_read(I2C_SLV_READ, 1, buff_rx);
    debug("I2C RD_E2(len=%d, ByteVal=0x%02X) -> %s\r\n",
          cnt, buff_rx[0], (buff_rx[0] == 0x5A) ? "OK" : "FAIL");

    // Stage 2: EEPROM block access
    debug("** Eeprom(0x%02X) RD/WR at_0x%04X \r\n", I2C_SLV_WRITE, AT24C64_DATA_ADDR);

    cnt = eeprom_read(I2C_SLV_WRITE, AT24C64_DATA_ADDR, DATA_BUFF_LEN, buff_rx);
    debug("Read(len=%d) before:\r\n", cnt);
    debugHex(buff_rx, cnt);

    for (uint16_t i = 0; i < DATA_BUFF_LEN; i++)
    {
        buff_tx[i] = buff_rx[i] + 2;
    }

    cnt = eeprom_write(I2C_SLV_WRITE, AT24C64_DATA_ADDR, DATA_BUFF_LEN, buff_tx);
    debug("Write(len=%d), autoInc 2...\r\n", cnt);
    bootDelayMs(EEPROM_WRITE_DELAY_MS);

    cnt = eeprom_read(I2C_SLV_WRITE, AT24C64_DATA_ADDR, DATA_BUFF_LEN, buff_rx);
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
    }
}
#endif

/**
 ****************************************************************************************
 * @brief I2C initialization
 *
 * @details
 * Configure I2C pins and clock. In slave mode, set address and enable ACK.
 ****************************************************************************************
 */
static void i2c_init_local(void)
{
    i2c_init(PIN_I2C_SCL, PIN_I2C_SDA, SCLK_100K);

    #if (I2C_ROLE_SLAVE)
    I2C_SET_ACK();
    I2C_SLV_ADR(I2C_SLV_ADDR);
    I2C_INT_EN();
    #endif
}

/**
 ****************************************************************************************
 * @brief I2C test entry point
 ****************************************************************************************
 */
void i2c_test(void)
{
    #if (FPGA_TEST)
    RCC->APB1CLK_DIV = 1;
    #endif

    debug("i2cTest Start...\r\n");

    i2c_init_local();
    i2c_proc();
}
