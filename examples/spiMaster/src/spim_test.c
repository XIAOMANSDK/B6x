/**
 ****************************************************************************************
 *
 * @file spim_test.c
 *
 * @brief Demo of SPI Master Usage.
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

#define BUFF_SIZE              16 

uint8_t tx_buff[BUFF_SIZE];
uint8_t rx_buff[BUFF_SIZE];


/*
 * FUNCTIONS
 ****************************************************************************************
 */

#if (SPI_DMA_MODE)

#define DMA_CHNL_TX            0
#define DMA_CHNL_RX            1

#if (SPI_FLASH_OP)
static void spimProc(void)
{ 
    // spim test
    debug("Read FlashID(cmd:0x9F, rxlen:3)\r\n");
    tx_buff[0] = 0x9F;
    DMA_SPIM_TX_CONF(DMA_CHNL_TX, tx_buff, 1+3, CCM_BASIC);
    DMA_SPIM_RX_CONF(DMA_CHNL_RX, rx_buff, 1+3, CCM_BASIC);
    SPIM_CS_L(SPI_CS_PAD);
    //spim_transimit(tx_buff, 1 + 3); 
    spim_begin(1+3); spim_wait();
    SPIM_CS_H(SPI_CS_PAD);
    debugHex(rx_buff, BUFF_SIZE);

    debug("Flash Status(cmd:0x05, rxlen:1)\r\n");
    tx_buff[0] = 0x05;
    DMA_SPIM_TX_CONF(DMA_CHNL_TX, tx_buff, 1+1, CCM_BASIC);
    DMA_SPIM_RX_CONF(DMA_CHNL_RX, rx_buff, 1+1, CCM_BASIC);
    SPIM_CS_L(SPI_CS_PAD);
    //spim_transimit(tx_buff, 1 + 1);
    spim_begin(1+1); spim_wait();
    SPIM_CS_H(SPI_CS_PAD);
    debugHex(rx_buff, BUFF_SIZE);
    
    debug("Flash RD(cmd:0x03, adr:0x000000, datlen:8)\r\n");
    tx_buff[0] = 0x03;
    tx_buff[1] = 0x00;
    tx_buff[2] = 0x00;
    tx_buff[3] = 0x00;
    DMA_SPIM_TX_CONF(DMA_CHNL_TX, tx_buff, 1+3+8, CCM_BASIC);
    DMA_SPIM_RX_CONF(DMA_CHNL_RX, rx_buff, 1+3+8, CCM_BASIC);
    SPIM_CS_L(SPI_CS_PAD);
    //spim_transimit(tx_buff, 1+3+8);
    spim_begin(1+3+8); spim_wait();
    SPIM_CS_H(SPI_CS_PAD);
    debugHex(rx_buff, BUFF_SIZE);
    
    while (1)
    {
        // empty
    };
}
#else
void spimProc(void)
{
    uint8_t tx_data = 0xFF; // 0xFF,0x00~0xFE
    uint8_t rx_data = 0xFF; // 0xFF,0x00~0xFE
    
    while (1)
    {
        for (uint8_t i = 0; i < BUFF_SIZE; i++)
        {
            tx_buff[i] = tx_data++;
        }
        
        DMA_SPIM_TX_CONF(DMA_CHNL_TX, tx_buff, BUFF_SIZE, CCM_BASIC);
        DMA_SPIM_RX_CONF(DMA_CHNL_RX, rx_buff, BUFF_SIZE, CCM_BASIC);
        SPIM_CS_L(SPI_CS_PAD);
        spim_begin(BUFF_SIZE); spim_wait();
        SPIM_CS_H(SPI_CS_PAD);
        
        debug("TX: %02X ~ %02X, RX:\r\n", tx_buff[0], tx_buff[BUFF_SIZE-1]);
        debugHex(rx_buff, BUFF_SIZE);
        
        for (uint8_t i = 0; i < BUFF_SIZE; i++)
        {
            if (rx_buff[i] != rx_data)
            {
                debug("RX Wrong(curr:%02X, expt:%02X)\r\n", rx_buff[i], rx_data);
                while(1);
            }
            rx_data++;
        }
    }
}
#endif

static void spimInit(void)
{
    dma_init();
    
    // spim init
    SPIM_CS_INIT(SPI_CS_PAD);
    spim_init(SPI_CLK_PAD, SPI_MISO_PAD, SPI_MOSI_PAD);
    // spim conf(dma mode)
    spim_conf(SPIM_CR_DFLT | SPIM_CR_RX_DMA_BIT | SPIM_CR_TX_DMA_BIT);
    
    // dma channel
    DMA_SPIM_TX_INIT(DMA_CHNL_TX);
    DMA_SPIM_RX_INIT(DMA_CHNL_RX);
}

#else //(SPIM_MCU_MODE)

#if (SPI_FLASH_OP)
#define HALF_DUPLEX        1

static void spimProc(void)
{
    // spim test
    debug("Read FlashID(cmd:0x9F, rxlen:3)\r\n");
    tx_buff[0] = 0x9F;
    SPIM_CS_L(SPI_CS_PAD);
    #if (HALF_DUPLEX)
    spim_halfdx(tx_buff, 1, rx_buff, 3);
    #else
    spim_duplex(tx_buff, rx_buff, 1 + 3);
    #endif
    SPIM_CS_H(SPI_CS_PAD);
    debugHex(rx_buff, 4);
    
    #if (SPIM_FSH_WR)
    debug("Flash WR_EN(cmd:0x06, rxlen:0)\r\n");
    tx_buff[0] = 0x06;
    SPIM_CS_L(SPI_CS_PAD);
    #if (HALF_DUPLEX)
    spim_halfdx(tx_buff, 1, rx_buff, 0);
    #else
    spim_duplex(tx_buff, rx_buff, 1);
    #endif
    SPIM_CS_H(SPI_CS_PAD);
    debugHex(rx_buff, 1);

    debug("Flash WR(cmd:0x02, adr:0x000000, datlen:4)\r\n");
    tx_buff[0] = 0x02;
    tx_buff[1] = 0x04;
    tx_buff[2] = 0x00;
    tx_buff[3] = 0x00;
    SPIM_CS_L(SPI_CS_PAD);
    #if (HALF_DUPLEX)
    spim_halfdx(tx_buff, 8, rx_buff, 0);
    #else
    spim_duplex(tx_buff, rx_buff, 1+3+4);
    #endif
    SPIM_CS_H(SPI_CS_PAD);
    debugHex(rx_buff, 8);
    
    bootDelayMs(10);
    #endif
    
    debug("Flash Status(cmd:0x05, rxlen:1)\r\n");
    tx_buff[0] = 0x05;
    SPIM_CS_L(SPI_CS_PAD);
    #if (HALF_DUPLEX)
    spim_halfdx(tx_buff, 1, rx_buff, 1);
    #else
    spim_duplex(tx_buff, rx_buff, 1 + 1);
    #endif
    SPIM_CS_H(SPI_CS_PAD);
    debugHex(rx_buff, 2);
    
    debug("Flash RD(cmd:0x03, adr:0x000000, datlen:8)\r\n");
    tx_buff[0] = 0x03;
    tx_buff[1] = 0x00;
    tx_buff[2] = 0x00;
    tx_buff[3] = 0x00;
    SPIM_CS_L(SPI_CS_PAD);
    #if (HALF_DUPLEX)
    spim_halfdx(tx_buff, 4, rx_buff, 8);
    #else
    spim_duplex(tx_buff, rx_buff, 1+3+8);
    #endif
    SPIM_CS_H(SPI_CS_PAD);
    debugHex(rx_buff, 8);
    
    while (1)
    {
        // empty
    };
}
#else
void spimProc(void)
{
    uint8_t tx_data = 0xFF; // 0xFF,0x00~0xFE
    uint8_t rx_data = 0xFF; // 0xFF,0x00~0xFE
    
    while (1)
    {
        for (uint8_t i = 0; i < BUFF_SIZE; i++)
        {
            tx_buff[i] = tx_data++;
        }
        
        SPIM_CS_L(SPI_CS_PAD);
        spim_duplex(tx_buff, rx_buff, BUFF_SIZE);
        SPIM_CS_H(SPI_CS_PAD);
        
        debug("TX: %02X ~ %02X, RX:\r\n", tx_buff[0], tx_buff[BUFF_SIZE-1]);
        debugHex(rx_buff, BUFF_SIZE);
        
        for (uint8_t i = 0; i < BUFF_SIZE; i++)
        {
            if (rx_buff[i] != rx_data)
            {
                debug("RX Wrong(curr:%02X, expt:%02X)\r\n", rx_buff[i], rx_data);
                while(1);
            }
            rx_data++;
        }
    }
}
#endif

static void spimInit(void)
{
    // spim init
    SPIM_CS_INIT(SPI_CS_PAD);
    spim_init(SPI_CLK_PAD, SPI_MISO_PAD, SPI_MOSI_PAD);

    // spim conf
    spim_conf(SPIM_CR_DFLT);
}

#endif

void spimTest(void)
{
    debug("spimTest Start...\r\n");
    
    // init buff
    for (uint8_t idx = 0; idx < BUFF_SIZE; idx++)
    {
        tx_buff[idx] = 0xFF;
        rx_buff[idx] = 0x00;
    }
    
    spimInit();
    
    spimProc();
}
