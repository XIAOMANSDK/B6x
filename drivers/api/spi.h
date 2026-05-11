/**
 ****************************************************************************************
 *
 * @file spi.h
 *
 * @brief Header file - SPI Driver
 *
 ****************************************************************************************
 */

#ifndef _SPI_H_
#define _SPI_H_

#include <stdint.h>
#include <stdbool.h>
#include "gpio.h"


/*
 * DEFINES
 ****************************************************************************************
 */

/// Bits field of SPI master control @see SPIM_CTRL_TypeDef
enum spim_ctrl_bfs
{
    // clock rate = clk/2^(crat+1), 0-11 is supported
    SPIM_CR_CLK_RATE_LSB       = 0,
    SPIM_CR_CLK_RATE_MSK       = (0x0F << SPIM_CR_CLK_RATE_LSB),
    
    // clock phase -- 0: sck sample in 1st edge, 1: sck sample in 2nd edge
    SPIM_CR_CLK_PHASE_BIT      = (1 << 4),
    // clock polarity -- 0: sck low in idle, 1: sck high in idle
    SPIM_CR_CLK_POLAR_BIT      = (1 << 5),
    
    // work mode -- 0: as mcu, 1: as dma mode
    SPIM_CR_TX_DMA_BIT         = (1 << 6),
    SPIM_CR_RX_DMA_BIT         = (1 << 7),
    
    // transfer mode -- 0: disable, 1: enable
    SPIM_CR_INT_EN_BIT         = (1 << 8),
    SPIM_CR_TX_EN_BIT          = (1 << 9),
    SPIM_CR_RX_EN_BIT          = (1 << 10),
    
    // bits order -- 0: LSB first, 1: MSB first
    SPIM_CR_MSB_FST_BIT        = (1 << 11),
};

#if (SYS_CLK == 1)
    #define SPIM_CR_DFLT         (SPIM_CR_MSB_FST_BIT | SPIM_CR_RX_EN_BIT | SPIM_CR_TX_EN_BIT | (SPIM_CR_CLK_RATE_MSK & 0x01))  // SPI 8MHz
#elif (SYS_CLK == 2)
    #define SPIM_CR_DFLT         (SPIM_CR_MSB_FST_BIT | SPIM_CR_RX_EN_BIT | SPIM_CR_TX_EN_BIT | (SPIM_CR_CLK_RATE_MSK & 0x02))  // SPI 6MHz
#elif (SYS_CLK == 3)
    #define SPIM_CR_DFLT         (SPIM_CR_MSB_FST_BIT | SPIM_CR_RX_EN_BIT | SPIM_CR_TX_EN_BIT | (SPIM_CR_CLK_RATE_MSK & 0x03))  // SPI 8MHz
#else
    #define SPIM_CR_DFLT         (SPIM_CR_MSB_FST_BIT | SPIM_CR_RX_EN_BIT | SPIM_CR_TX_EN_BIT)  // SPI 8MHz
#endif //SYS_CLK

enum spim_staclr_bfs
{
    SPIM_TXDAT_CLR_BIT         = (1 << 0),
    SPIM_RXDAT_CLR_BIT         = (1 << 1),
    SPIM_INTFLG_CLR_BIT        = (1 << 2),
    
    SPIM_STATUS_CLR_ALL        = 0x07,
};

#define SPIM_CS_H(pad)           GPIO_DAT_SET(1UL << (pad))
#define SPIM_CS_L(pad)           GPIO_DAT_CLR(1UL << (pad))

#define SPIM_CS_INIT(pad)               \
    dowl(                               \
        GPIO_DAT_SET(1UL << (pad));     \
        GPIO_DIR_SET(1UL << (pad));     \
        iom_ctrl(pad, IOM_SEL_GPIO);    \
    )

/// Bits field of SPI slave control @see SPIS_CTRL_TypeDef
enum spis_ctrl_bfs
{
    // bits order -- 0: MSB first, 1: LSB first
    SPIS_CR_LSB_FST_BIT        = (1 << 0),
    // clock polarity -- 0: sck low in idle, 1: sck high in idle
    SPIS_CR_CLK_POLAR_BIT      = (1 << 1),
    // clock phase -- 0: sck sample in 1st edge, 1: sck sample in 2nd edge
    SPIS_CR_CLK_PHASE_BIT      = (1 << 2),
    // transfer mode -- 0: disable, 1: enable
    SPIS_CR_RX_EN_BIT          = (1 << 3),
    SPIS_CR_RXINT_EN_BIT       = (1 << 4),
    // work mode -- 0: as mcu, 1: as mode
    SPIS_CR_TX_DMA_BIT         = (1 << 5),
    SPIS_CR_RX_DMA_BIT         = (1 << 6),
    // work enable -- 0: disable(spis lsbfst spol and spha can be configured only when it is 0)
    SPIS_CR_RUN_EN_BIT         = (1 << 7),
    
    // overtime window -- unit in bit width time
    SPIS_CR_OT_WIN_LSB         = 8,
    SPIS_CR_OT_WIN_MSK         = (0xFF << SPIS_CR_OT_WIN_LSB),
    SPIS_CR_OT_EN_BIT          = (1 << 16),
    SPIS_CR_OTINT_EN_BIT       = (1 << 17),
};

#define SPIS_CR_DFLT             (SPIS_CR_RX_EN_BIT | SPIS_CR_RUN_EN_BIT)

enum spis_infclr_bfs
{
    SPIS_RXOVER_CLR_BIT        = (1 << 0),
    SPIS_TXDAT_CLR_BIT         = (1 << 1),
    SPIS_RXDAT_CLR_BIT         = (1 << 2),
    SPIS_RXINT_CLR_BIT         = (1 << 3),
    SPIS_OTINT_CLR_BIT         = (1 << 4),
    
    SPIS_INFO_CLR_ALL          = 0x1F,
};


/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */
 
/**
 ****************************************************************************************
 * @brief Init SPI Master Module.
 *
 * @param[in] io_clk   io used for SPI clk @see enum pad_idx.
 * @param[in] io_miso  io used for SPI miso @see enum pad_idx.
 * @param[in] io_mosi  io used for SPI mosi @see enum pad_idx.
 * 
 * @note User Control CS IO. Macro SPIM_CS_H & SPIM_CS_L
 ****************************************************************************************
 */
void spim_init(uint8_t io_clk, uint8_t io_miso, uint8_t io_mosi);

/**
 ****************************************************************************************
 * @brief Config SPI Master.
 *
 * @param[in] ctrl   Bits field of value @see enum spim_ctrl_bfs.
 ****************************************************************************************
 */
void spim_conf(uint32_t ctrl);

/**
 ****************************************************************************************
 * @brief SPI Master Begin.
 *
 * @param[in] data_len   The number of byte transimit/receive from SPI.
 ****************************************************************************************
 */
void spim_begin(uint16_t data_len);

/**
 ****************************************************************************************
 * @brief SPI Master Wait Idle.
 ****************************************************************************************
 */
void spim_wait(void);

/**
 ****************************************************************************************
 * @brief SPI Master Duplex.
 *
 * @param[in] tx_data   SPI transimit data pointer.
 * @param[in] rx_buff   SPI receive buff pointer.
 * @param[in] data_len  The number of byte receive from SPI.
 ****************************************************************************************
 */
void spim_duplex(const uint8_t *tx_data, uint8_t *rx_buff, uint16_t data_len);

/**
 ****************************************************************************************
 * @brief SPI Master transimit data.
 *
 * @param[in] tx_data   SPI transimit data pointer.
 * @param[in] data_len  The number of byte transimit to SPI.
 ****************************************************************************************
 */
void spim_transimit(const uint8_t *tx_data, uint16_t length);

/**
 ****************************************************************************************
 * @brief SPI Master receive data.
 *
 * @param[in] rx_buff   SPI receive buff pointer.
 * @param[in] data_len  The number of byte receive from SPI.
 ****************************************************************************************
 */
void spim_receive(uint8_t *rx_buff, uint16_t length);

/**
 ****************************************************************************************
 * @brief SPI Master Half-Duplex.
 *
 * @param[in] cmd   SPI transimit data pointer.
 * @param[in] clen  The number of byte transimit to SPI.
 * @param[in] rsp   SPI receive buff pointer.
 * @param[in] rlen  The number of byte receive from SPI.
 ****************************************************************************************
 */
void spim_halfdx(const uint8_t *cmd, uint16_t clen, uint8_t *rsp, uint16_t rlen);



/**
 ****************************************************************************************
 * @brief Init SPI Slave Module.
 *
 * @param[in] io_cs    io used for SPI cs @see enum pad_idx.
 * @param[in] io_clk   io used for SPI clk @see enum pad_idx.
 * @param[in] io_miso  io used for SPI miso @see enum pad_idx.
 * @param[in] io_mosi  io used for SPI mosi @see enum pad_idx.
 ****************************************************************************************
 */
void spis_init(uint8_t io_cs, uint8_t io_clk, uint8_t io_miso, uint8_t io_mosi);

/**
 ****************************************************************************************
 * @brief Config SPI Slave.
 *
 * @param[in] ctrl   Bits field of value @see enum spis_ctrl_bfs.
 ****************************************************************************************
 */
void spis_conf(uint32_t ctrl);

/**
 ****************************************************************************************
 * @brief SPI Slave get the received data.
 *
 * @param[in] ch  received data.
 *
 * @return true   received data success. false   received data failed.
 ****************************************************************************************
 */
bool spis_getc(uint8_t *ch);

/**
 ****************************************************************************************
 * @brief SPI Slave Transmits one byte data.
 *
 * @param[in] ch    the data transmited
 *
 ****************************************************************************************
 */
void spis_putc(uint8_t ch);

#endif // _SPI_H_
