/**
 ****************************************************************************************
 *
 * @file spim_test.c
 *
 * @brief SPI Master test with DMA/MCU and Flash/Custom modes
 *
 * @details
 * Compile-time configuration via cfg.h:
 * - SPI_DMA_MODE: 1=DMA, 0=MCU polling
 * - SPI_FLASH_OP: 1=SPI Flash operations, 0=Custom data loopback
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "drvs.h"
#include "regs.h"
#include "dbg.h"
#include <string.h>


/*
 * DEFINES
 ****************************************************************************************
 */

#define BUFF_SIZE              128  ///< Transfer buffer size

/// Flash address bytes (24-bit)
#define FLASH_ADDR_H           0x00
#define FLASH_ADDR_M           0x00
#define FLASH_ADDR_L           0x00


/*
 * GLOBALS
 ****************************************************************************************
 */

static uint8_t tx_buff[BUFF_SIZE];  ///< TX data buffer
static uint8_t rx_buff[BUFF_SIZE];  ///< RX data buffer


/*
 * FUNCTIONS
 ****************************************************************************************
 */

#if (SPI_DMA_MODE)

#define DMA_CHNL_TX            0    ///< DMA TX channel
#define DMA_CHNL_RX            1    ///< DMA RX channel

/**
 ****************************************************************************************
 * @brief Perform SPI DMA transfer (CS assert + DMA + wait + CS deassert)
 *
 * @param[in] len Transfer length in bytes
 ****************************************************************************************
 */
static void spim_dma_xfer(uint16_t len)
{
    DMA_SPIM_TX_CONF(DMA_CHNL_TX, tx_buff, len, CCM_BASIC);
    DMA_SPIM_RX_CONF(DMA_CHNL_RX, rx_buff, len, CCM_BASIC);

    SPIM_CS_L(SPI_CS_PAD);
    spim_begin(len);
    spim_wait();
    SPIM_CS_H(SPI_CS_PAD);
}

#if (SPI_FLASH_OP)

/**
 ****************************************************************************************
 * @brief SPI Master Flash operations (DMA mode)
 *
 * @details Reads Flash ID, status, performs erase/write/read with verification
 ****************************************************************************************
 */
static void spim_proc(void)
{
    // Read Flash ID
    debug("Read FlashID(cmd:0x9F, rxlen:3)\r\n");
    tx_buff[0] = FLASH_CMD_READ_ID;
    spim_dma_xfer(1 + 3);
    debugHex(rx_buff, 4);

    // Read Flash Status
    debug("Flash Status(cmd:0x05, rxlen:1)\r\n");
    tx_buff[0] = FLASH_CMD_READ_STATUS;
    spim_dma_xfer(1 + 1);
    debugHex(rx_buff, 2);

    // Chip Erase
    debug("Flash ER(cmd:0x60, rxlen:1)\r\n");
    tx_buff[0] = FLASH_CMD_CHIP_ERASE;
    spim_dma_xfer(1);
    debugHex(rx_buff, 1);

    // Write Enable
    debug("Flash WR EN(cmd:0x06, rxlen:1)\r\n");
    tx_buff[0] = FLASH_CMD_WRITE_ENABLE;
    spim_dma_xfer(1);
    debugHex(rx_buff, 1);

    // Page Program
    uint8_t data_len = BUFF_SIZE - 4;
    debug("Flash WR(cmd:0x02, adr:0x000000, datlen:%d)\r\n", data_len);
    tx_buff[0] = FLASH_CMD_PAGE_PROGRAM;
    tx_buff[1] = FLASH_ADDR_H;
    tx_buff[2] = FLASH_ADDR_M;
    tx_buff[3] = FLASH_ADDR_L;

    for (uint8_t i = 0; i < data_len; i++)
    {
        tx_buff[4 + i] = i + 1;
    }

    spim_dma_xfer(4 + data_len);
    debugHex(rx_buff, 4 + data_len);
    memset(tx_buff, 0x00, BUFF_SIZE);

    // Read Data
    debug("Flash RD(cmd:0x03, adr:0x000000, datlen:%d)\r\n", data_len);
    tx_buff[0] = FLASH_CMD_READ_DATA;
    tx_buff[1] = FLASH_ADDR_H;
    tx_buff[2] = FLASH_ADDR_M;
    tx_buff[3] = FLASH_ADDR_L;
    spim_dma_xfer(4 + data_len);
    debugHex(rx_buff, 4 + data_len);

    // Continuous random-length read test
    while (1)
    {
        data_len = (sadc_rand_num()) % (BUFF_SIZE - 4);
        debug("Flash RD(cmd:0x03, adr:0x000000, datlen:%d)\r\n", data_len);
        tx_buff[0] = FLASH_CMD_READ_DATA;
        tx_buff[1] = FLASH_ADDR_H;
        tx_buff[2] = FLASH_ADDR_M;
        tx_buff[3] = FLASH_ADDR_L;
        spim_dma_xfer(4 + data_len);
        debugHex(rx_buff, 4 + data_len);

        // Verify received data
        for (uint8_t i = 0; i < data_len; i++)
        {
            if (rx_buff[4 + i] != i + 1)
                while (1);
        }

        memset(rx_buff, 0, BUFF_SIZE);
    }
}

#else // SPI_FLASH_OP == 0

/**
 ****************************************************************************************
 * @brief SPI Master custom data transfer (DMA mode)
 *
 * @details Continuous full-duplex transfer with data verification
 ****************************************************************************************
 */
static void spim_proc(void)
{
    uint8_t tx_data = 0xFF;
    uint8_t rx_data = 0xFF;

    while (1)
    {
        for (uint8_t i = 0; i < BUFF_SIZE; i++)
        {
            tx_buff[i] = tx_data++;
        }

        spim_dma_xfer(BUFF_SIZE);

        debug("TX: %02X ~ %02X, RX:\r\n", tx_buff[0], tx_buff[BUFF_SIZE - 1]);
        debugHex(rx_buff, BUFF_SIZE);

        for (uint8_t i = 0; i < BUFF_SIZE; i++)
        {
            if (rx_buff[i] != rx_data)
            {
                debug("RX Wrong(curr:%02X, expt:%02X)\r\n", rx_buff[i], rx_data);
                while (1);
            }
            rx_data++;
        }
    }
}

#endif // SPI_FLASH_OP

/**
 ****************************************************************************************
 * @brief SPI Master initialization (DMA mode)
 ****************************************************************************************
 */
static void spim_port_init(void)
{
    dma_init();
    SPIM_CS_INIT(SPI_CS_PAD);
    spim_init(SPI_CLK_PAD, SPI_MISO_PAD, SPI_MOSI_PAD);
    spim_conf(SPIM_CR_DFLT | SPIM_CR_RX_DMA_BIT | SPIM_CR_TX_DMA_BIT);

    DMA_SPIM_TX_INIT(DMA_CHNL_TX);
    DMA_SPIM_RX_INIT(DMA_CHNL_RX);
}

#else // SPI_DMA_MODE == 0 (MCU mode)

#if (SPI_FLASH_OP)

#define HALF_DUPLEX            1    ///< Half-duplex mode flag

/**
 ****************************************************************************************
 * @brief SPI Master Flash operations (MCU mode)
 ****************************************************************************************
 */
static void spim_proc(void)
{
    // Read Flash ID
    debug("Read FlashID(cmd:0x9F, rxlen:3)\r\n");
    tx_buff[0] = FLASH_CMD_READ_ID;
    SPIM_CS_L(SPI_CS_PAD);
    #if (HALF_DUPLEX)
    spim_halfdx(tx_buff, 1, rx_buff, 3);
    #else
    spim_duplex(tx_buff, rx_buff, 1 + 3);
    #endif
    SPIM_CS_H(SPI_CS_PAD);
    debugHex(rx_buff, 4);

    #if (SPIM_FSH_WR)
    // Write Enable
    debug("Flash WR_EN(cmd:0x06, rxlen:0)\r\n");
    tx_buff[0] = FLASH_CMD_WRITE_ENABLE;
    SPIM_CS_L(SPI_CS_PAD);
    #if (HALF_DUPLEX)
    spim_halfdx(tx_buff, 1, rx_buff, 0);
    #else
    spim_duplex(tx_buff, rx_buff, 1);
    #endif
    SPIM_CS_H(SPI_CS_PAD);
    debugHex(rx_buff, 1);

    // Page Program
    debug("Flash WR(cmd:0x02, adr:0x000400, datlen:4)\r\n");
    tx_buff[0] = FLASH_CMD_PAGE_PROGRAM;
    tx_buff[1] = 0x04;
    tx_buff[2] = 0x00;
    tx_buff[3] = 0x00;
    SPIM_CS_L(SPI_CS_PAD);
    #if (HALF_DUPLEX)
    spim_halfdx(tx_buff, 8, rx_buff, 0);
    #else
    spim_duplex(tx_buff, rx_buff, 1 + 3 + 4);
    #endif
    SPIM_CS_H(SPI_CS_PAD);
    debugHex(rx_buff, 8);
    bootDelayMs(10);
    #endif // SPIM_FSH_WR

    // Read Status
    debug("Flash Status(cmd:0x05, rxlen:1)\r\n");
    tx_buff[0] = FLASH_CMD_READ_STATUS;
    SPIM_CS_L(SPI_CS_PAD);
    #if (HALF_DUPLEX)
    spim_halfdx(tx_buff, 1, rx_buff, 1);
    #else
    spim_duplex(tx_buff, rx_buff, 1 + 1);
    #endif
    SPIM_CS_H(SPI_CS_PAD);
    debugHex(rx_buff, 2);

    // Read Data
    debug("Flash RD(cmd:0x03, adr:0x000000, datlen:8)\r\n");
    tx_buff[0] = FLASH_CMD_READ_DATA;
    tx_buff[1] = FLASH_ADDR_H;
    tx_buff[2] = FLASH_ADDR_M;
    tx_buff[3] = FLASH_ADDR_L;
    SPIM_CS_L(SPI_CS_PAD);
    #if (HALF_DUPLEX)
    spim_halfdx(tx_buff, 4, rx_buff, 8);
    #else
    spim_duplex(tx_buff, rx_buff, 1 + 3 + 8);
    #endif
    SPIM_CS_H(SPI_CS_PAD);
    debugHex(rx_buff, 8);

    while (1)
    {
    }
}

#else // SPI_FLASH_OP == 0

/**
 ****************************************************************************************
 * @brief SPI Master custom data transfer (MCU mode)
 ****************************************************************************************
 */
static void spim_proc(void)
{
    uint8_t tx_data = 0xFF;
    uint8_t rx_data = 0xFF;

    while (1)
    {
        for (uint8_t i = 0; i < BUFF_SIZE; i++)
        {
            tx_buff[i] = tx_data++;
        }

        SPIM_CS_L(SPI_CS_PAD);
        spim_duplex(tx_buff, rx_buff, BUFF_SIZE);
        SPIM_CS_H(SPI_CS_PAD);

        debug("TX: %02X ~ %02X, RX:\r\n", tx_buff[0], tx_buff[BUFF_SIZE - 1]);
        debugHex(rx_buff, BUFF_SIZE);

        for (uint8_t i = 0; i < BUFF_SIZE; i++)
        {
            if (rx_buff[i] != rx_data)
            {
                debug("RX Wrong(curr:%02X, expt:%02X)\r\n", rx_buff[i], rx_data);
                while (1);
            }
            rx_data++;
        }
    }
}

#endif // SPI_FLASH_OP

/**
 ****************************************************************************************
 * @brief SPI Master initialization (MCU mode)
 ****************************************************************************************
 */
static void spim_port_init(void)
{
    SPIM_CS_INIT(SPI_CS_PAD);
    spim_init(SPI_CLK_PAD, SPI_MISO_PAD, SPI_MOSI_PAD);
    spim_conf(SPIM_CR_DFLT);
}

#endif // SPI_DMA_MODE

/**
 ****************************************************************************************
 * @brief SPI Master test entry point
 ****************************************************************************************
 */
void spim_test(void)
{
    debug("spimTest Start...\r\n");

    memset(tx_buff, 0xFF, BUFF_SIZE);
    memset(rx_buff, 0x00, BUFF_SIZE);

    spim_port_init();
    spim_proc();
}
