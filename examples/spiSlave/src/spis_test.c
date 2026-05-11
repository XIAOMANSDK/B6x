/**
 ****************************************************************************************
 *
 * @file spis_test.c
 *
 * @brief SPI Slave test with DMA/MCU modes
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "drvs.h"
#include "dbg.h"


#if (SPI_DMA_MODE)

#define DMA_CHNL_TX            0    ///< DMA TX channel
#define DMA_CHNL_RX            1    ///< DMA RX channel

#define BUFF_SIZE              16   ///< Transfer buffer size

static uint8_t tx_buff[BUFF_SIZE];  ///< TX data buffer
static uint8_t rx_buff[BUFF_SIZE];  ///< RX data buffer

/**
 ****************************************************************************************
 * @brief SPI Slave DMA mode processing
 *
 * @details Full-duplex transfer with DMA, verifies received data sequence
 ****************************************************************************************
 */
static void spis_proc(void)
{
    // Slave sends 0xFF by default when connected; master sends 0xFF first, then TX/RX keep same sequence
    uint8_t tx_data = 0x00;
    uint8_t rx_data = 0xFF;

    // Fill TX buffer with initial data
    for (uint8_t i = 0; i < BUFF_SIZE; i++)
    {
        tx_buff[i] = tx_data++;
    }
    
    DMA_SPIS_RX_CONF(DMA_CHNL_RX, rx_buff, BUFF_SIZE, CCM_BASIC);
    DMA_SPIS_TX_CONF(DMA_CHNL_TX, tx_buff, BUFF_SIZE, CCM_BASIC);

    debug("DMA Remain(TX:%d,RX:%d)\r\n", dma_chnl_remain(DMA_CHNL_TX), dma_chnl_remain(DMA_CHNL_RX));
    
    while (1)
    {
        if (dma_chnl_done(DMA_CHNL_RX))
        {
            for (uint8_t i = 0; i < BUFF_SIZE; i++)
            {
                if (rx_buff[i] != rx_data)
                {
                    debug("RX Wrong(curr:%02X, expt:%02X)\r\n", rx_buff[i], rx_data);
                    debugHex(rx_buff, BUFF_SIZE);
                    while (1);
                }
                rx_data++;
            }
            
            dma_chnl_reload(DMA_CHNL_RX);
        }

        if (dma_chnl_done(DMA_CHNL_TX))
        {
            for (uint8_t i = 0; i < BUFF_SIZE; i++)
            {
                tx_buff[i] = tx_data++;
            }
            
            dma_chnl_reload(DMA_CHNL_TX);
        }
    }
}

/**
 ****************************************************************************************
 * @brief SPI Slave initialization (DMA mode)
 ****************************************************************************************
 */
static void spis_setup(void)
{
    dma_init();
    spis_init(SPI_CS_PAD, SPI_CLK_PAD, SPI_MISO_PAD, SPI_MOSI_PAD);
    spis_conf(SPIS_CR_DFLT | SPIS_CR_TX_DMA_BIT | SPIS_CR_RX_DMA_BIT);
    DMA_SPIS_TX_INIT(DMA_CHNL_TX);
    DMA_SPIS_RX_INIT(DMA_CHNL_RX);
}

#else // SPI_DMA_MODE == 0 (MCU mode)

/**
 ****************************************************************************************
 * @brief SPI Slave MCU mode processing
 *
 * @details Byte-by-byte transfer via polling
 ****************************************************************************************
 */
static void spis_proc(void)
{
    // Slave sends 0xFF by default; master sends 0xFF first, then TX/RX keep same sequence
    uint8_t tx_data = 0x00;
    uint8_t rx_data = 0xFF;
    uint8_t rx_curr;

    debug("SPI getc & putc\r\n");

    while (1)
    {
        if (spis_getc(&rx_curr))
        {
            if (rx_curr != rx_data)
            {
                debug("RX Wrong(curr:%02X, expt:%02X), TX:%02X\r\n", rx_curr, rx_data, tx_data);
                while (1);
            }
            
            rx_data++;
        }
        
        spis_putc(tx_data++);
    }
}

/**
 ****************************************************************************************
 * @brief SPI Slave initialization (MCU mode)
 ****************************************************************************************
 */
static void spis_setup(void)
{
    spis_init(SPI_CS_PAD, SPI_CLK_PAD, SPI_MISO_PAD, SPI_MOSI_PAD);
    spis_conf(SPIS_CR_DFLT);
}

#endif

/**
 ****************************************************************************************
 * @brief SPI Slave test entry point
 ****************************************************************************************
 */
void spis_test(void)
{
    debug("spisTest Start...\r\n");

    spis_setup();
    spis_proc();
}
