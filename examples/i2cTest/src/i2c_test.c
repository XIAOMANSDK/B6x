/**
 ****************************************************************************************
 *
 * @file i2c_test.c
 *
 * @brief Demo of I2C Master/Slave Usage.
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

#define PIN_I2C_SCL            PA11
#define PIN_I2C_SDA            PA12
#define GPIO_I2C_TEST          GPIO02

#define I2C_SLV_ADDR           0x57 // AT24C64 Eeprom
#define I2C_SLV_WRITE          ((I2C_SLV_ADDR << 1) | 0x0)
#define I2C_SLV_READ           ((I2C_SLV_ADDR << 1) | 0x1)

#define AT24C64_EEP            ((I2C_SLV_ADDR << 1))

#define AT24C64_DATA_ADDR      0x20

#define DATA_BUFF_LEN          16


/*
 * FUNCTIONS
 ****************************************************************************************
 */

#if (I2C_ROLE_SLAVE)

#define MEM_ADDR_BASE 0x20008000

uint8_t dataBuff[258] = {0};
uint16_t buff_pos = 0;
uint32_t mem_addr = 0;

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
        
        } break;
        
        case I2C_SS_RECV_DAT_ACK:
        {
             I2C_DATA_GET(dataBuff[buff_pos++]);
        } break;
        
        case I2C_SS_RECV_STOP_OR_RESTART:
        {
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
            I2C_DATA_PUT(*(uint8_t *)(mem_addr++));
        } break;
    }

    I2C_IFLG_CLR();
    
    NVIC_ClearPendingIRQ(I2C_IRQn);
    NVIC_EnableIRQ(I2C_IRQn);
}

void I2C_IRQHandler(void)
{
    NVIC_DisableIRQ(I2C_IRQn);
    
    //i2c_isr();
}

static void i2cProc(void)
{
    while (1)
    {
        i2c_isr(); // polling
    }
}

#else // (I2C_ROLE_MASTER)

static void i2cProc(void)
{
    uint16_t cnt;
    uint8_t buff_tx[DATA_BUFF_LEN];
    uint8_t buff_rx[DATA_BUFF_LEN];

    // I2C access as eeprom
    debug("** I2C Master RD(0x%02X)/WR(0x%02X) 1B \r\n", I2C_SLV_READ, I2C_SLV_WRITE);
    
    buff_tx[0] = (AT24C64_DATA_ADDR >> 8) & 0xFF;
    buff_tx[1] = AT24C64_DATA_ADDR & 0xFF; // data addr MSB
    buff_tx[2] = 0x5A; // 1B data
    
    i2c_write(I2C_SLV_WRITE, 3, buff_tx); // write 1byte (3len-2addr)
    debug("I2C WR_E2(ByteVal=0x5A)...\r\n");
    bootDelayMs(60);
    
    i2c_write(I2C_SLV_WRITE, 2, buff_tx); // data addr MSB
    cnt = i2c_read(I2C_SLV_READ, 1, buff_rx);
    debug("I2C RD_E2(len=%d, ByteVal=0x%02X) -> %s\r\n", cnt, buff_rx[0], (buff_rx[0]==0x5A)?"OK":"FAIL");
    
    // Eeprom Test
    debug("** Eeprom(0x%02X) RD/WR at_0x%04X \r\n", AT24C64_EEP, AT24C64_DATA_ADDR);
    
    cnt = eeprom_read(AT24C64_EEP, AT24C64_DATA_ADDR, DATA_BUFF_LEN, buff_rx);
    debug("Read(len=%d) before:\r\n", cnt);
    debugHex(buff_rx, cnt);
    
    for (uint16_t i = 0; i < DATA_BUFF_LEN; i++)
    {
        buff_tx[i] = buff_rx[i] + 2; // autoInc
    }
    
    cnt = eeprom_write(AT24C64_EEP, AT24C64_DATA_ADDR, DATA_BUFF_LEN, buff_tx);
    debug("Write(len=%d), autoInc 2...\r\n", cnt);
    bootDelayMs(60); // delay 10ms, wait write complete

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
        // empty
    };
}
#endif

static void i2cInit(void)
{
    i2c_init(PIN_I2C_SCL, PIN_I2C_SDA, SCLK_100K);
    
    #if (I2C_ROLE_SLAVE)
    I2C_SET_ACK();
    I2C_SLV_ADR(0x57);

    // Enable interrupt
    I2C_INT_EN();
    
    NVIC_ClearPendingIRQ(I2C_IRQn);
    NVIC_EnableIRQ(I2C_IRQn);
    __enable_irq();
    #endif
}

void i2cTest(void)
{
    #if (FPGA_TEST)
    // FPGA 48M  need use 24M 20221119 --whl
    RCC->APB1CLK_DIV = 1;
    #endif
    
    debug("i2cTest Start...\r\n"); 

    i2cInit();

    i2cProc();
}
