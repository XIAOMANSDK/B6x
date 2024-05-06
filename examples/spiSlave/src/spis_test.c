/**
 ****************************************************************************************
 *
 * @file spis_test.c
 *
 * @brief Demo of SPI Slave Usage.
 *
 ****************************************************************************************
 */

#include "b6x.h"
#include "drvs.h"
#include "dbg.h"


#if (SPI_DMA_MODE)

/// Channel of DMA
#define DMA_CHNL_TX            0
#define DMA_CHNL_RX            1

#define BUFF_SIZE              16

uint8_t tx_buff[BUFF_SIZE];
uint8_t rx_buff[BUFF_SIZE];

static void spisProc(void)
{
    // slave auto-send 0xFF when connect, so master send 0xFF first, then TX/RX 0x00~0xFF keep same!
    uint8_t tx_data = 0x00; 
    uint8_t rx_data = 0xFF;

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

static void spisInit(void)
{
    dma_init();

    // spis init
    spis_init(SPI_CS_PAD, SPI_CLK_PAD, SPI_MISO_PAD, SPI_MOSI_PAD);  
    
    // spis conf
    spis_conf(SPIS_CR_DFLT | SPIS_CR_TX_DMA_BIT | SPIS_CR_RX_DMA_BIT);
    
    // dma channel
    DMA_SPIS_TX_INIT(DMA_CHNL_TX);
    DMA_SPIS_RX_INIT(DMA_CHNL_RX);
}

#else //(SPIM_MCU_MODE)

static void spisProc(void)
{
    // slave auto-send 0xFF when connect, so master send 0xFF first, then TX/RX 0x00~0xFF keep same!
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

static void spisInit(void)
{
    // spis init
    spis_init(SPI_CS_PAD, SPI_CLK_PAD, SPI_MISO_PAD, SPI_MOSI_PAD);  
    
    // spis conf
    spis_conf(SPIS_CR_DFLT);
}

#endif

void spisTest(void)
{
    debug("spisTest Start...\r\n");
    
    spisInit();
    
    spisProc();
}
