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

#define NSG_BYTE        (0xFF) // Non-significant data

#define SPIM_BGN(len)   dowl( SPIM->DAT_LEN = (len); SPIM->TXRX_BGN = 1; )
#define SPIM_WAIT()     dowl( while (SPIM->STATUS.SPIM_BUSY); SPIM->STATUS_CLR.Word = SPIM_STATUS_CLR_ALL; )


/*
 * FUNCTIONS - SPI Master
 ****************************************************************************************
 */

void spim_init(uint8_t io_clk, uint8_t io_miso, uint8_t io_mosi)
{
    // i2c_clk_en rst_req or soft_reset
    RCC_AHBCLK_EN(AHB_SPIM_BIT);
    RCC_AHBRST_REQ(AHB_SPIM_BIT);
    
    // csc connect
    csc_output(io_clk,  CSC_SPIM_CLK);
    csc_input(io_miso,  CSC_SPIM_MISO);
    csc_output(io_mosi, CSC_SPIM_MOSI);
    
    // iomode control
    iom_ctrl(io_clk,  IOM_SEL_CSC | IOM_DRV_LVL1 | IOM_PULLUP);
    iom_ctrl(io_miso, IOM_SEL_CSC | IOM_PULLUP   | IOM_INPUT);
    iom_ctrl(io_mosi, IOM_SEL_CSC | IOM_DRV_LVL1 | IOM_PULLUP);
}

void spim_conf(uint32_t ctrl)
{
    // clear FiFo and inner counter
    SPIM->STATUS_CLR.Word = SPIM_STATUS_CLR_ALL;
    
    // config ctrl
    SPIM->CTRL.Word = ctrl;
}

void spim_begin(uint16_t data_len)
{
    //if (SPIM->STATUS.SPIM_BUSY == 0) - rm, dma mode not fit
    SPIM_BGN(data_len);
}

void spim_wait(void)
{
    SPIM_WAIT();
}

void spim_duplex(const uint8_t *tx_data, uint8_t *rx_buff, uint16_t length)
{
    uint32_t tidx, ridx;

    SPIM_BGN(length);
    
    for (tidx = 0, ridx = 0; ridx < length; )
    {
        if ((tidx < length) && (SPIM->STATUS.SPIM_TX_FFULL == 0))
        {
            SPIM->TX_DATA = tx_data[tidx++];
        }
        
        if (SPIM->STATUS.SPIM_RX_FEMPTY == 0)
        {
            rx_buff[ridx++] = SPIM->RX_DATA;
        }
    }
    
    SPIM_WAIT();
}

void spim_transimit(const uint8_t *tx_data, uint16_t length)
{
    uint32_t tidx, ridx;;
    
    SPIM_BGN(length);

    // send cmd
    for (tidx = 0, ridx = 0; ridx < length; )
    {
        if ((tidx < length) && (SPIM->STATUS.SPIM_TX_FFULL == 0))
        {
            SPIM->TX_DATA = tx_data[tidx++];
        }
        
        if (SPIM->STATUS.SPIM_RX_FEMPTY == 0)
        {
            SPIM->RX_DATA; // drop
            ridx++;
        }
    }
    
    SPIM_WAIT();
}

void spim_receive(uint8_t *rx_buff, uint16_t length)
{
    uint32_t tidx, ridx;
    
    SPIM_BGN(length);
    
    // recv rsp
    for (tidx = 0, ridx = 0; ridx < length; )
    {
        if ((tidx < length) && (SPIM->STATUS.SPIM_TX_FFULL == 0))
        {
            SPIM->TX_DATA = NSG_BYTE;
            tidx++;
        }
        
        if (SPIM->STATUS.SPIM_RX_FEMPTY == 0)
        {
            rx_buff[ridx++] = SPIM->RX_DATA;
        }
    }
    
    SPIM_WAIT();
}

void spim_halfdx(const uint8_t *cmd, uint16_t clen, uint8_t *rsp, uint16_t rlen)
{
    uint32_t tidx, ridx;
    
    SPIM_BGN(clen + rlen);
    
    // send cmd
    for (tidx = 0, ridx = 0; ridx < clen; )
    {
        if ((tidx < clen) && (SPIM->STATUS.SPIM_TX_FFULL == 0))
        {
            SPIM->TX_DATA = cmd[tidx++];
        }
        
        if (SPIM->STATUS.SPIM_RX_FEMPTY == 0)
        {
            SPIM->RX_DATA; // drop
            ridx++;
        }
    }
    
    // recv rsp
    for (tidx = 0, ridx = 0; ridx < rlen; )
    {
        if ((tidx < rlen) && (SPIM->STATUS.SPIM_TX_FFULL == 0))
        {
            SPIM->TX_DATA = NSG_BYTE;
            tidx++;
        }
        
        if (SPIM->STATUS.SPIM_RX_FEMPTY == 0)
        {
            rsp[ridx++] = SPIM->RX_DATA;
        }
    }
    
    SPIM_WAIT();
}

/*
 * FUNCTIONS - SPI Slave
 ****************************************************************************************
 */

void spis_init(uint8_t io_cs, uint8_t io_clk, uint8_t io_miso, uint8_t io_mosi)
{
    // i2c_clk_en rst_req or soft_reset
    RCC_AHBCLK_EN(AHB_SPIS_BIT);
    RCC_AHBRST_REQ(AHB_SPIS_BIT);
    
    // csc connect
    csc_input(io_cs,    CSC_SPIS_CSN);
    csc_input(io_clk,   CSC_SPIS_CLK);
    csc_output(io_miso, CSC_SPIS_MISO);
    csc_input(io_mosi,  CSC_SPIS_MOSI);
    
    // iomode control
    iom_ctrl(io_cs,   IOM_SEL_CSC | IOM_PULLUP   | IOM_INPUT);
    iom_ctrl(io_clk,  IOM_SEL_CSC | IOM_PULLUP   | IOM_INPUT);
    iom_ctrl(io_miso, IOM_SEL_CSC | IOM_DRV_LVL1 | IOM_PULLUP);
    iom_ctrl(io_mosi, IOM_SEL_CSC | IOM_PULLUP   | IOM_INPUT);
}

void spis_conf(uint32_t ctrl)
{
    // disable run
    SPIS->CTRL.Word = 0;
    // clear FiFo and Interrupt
    SPIS->INFO_CLR.Word = SPIS_INFO_CLR_ALL;
    
    // config ctrl
    SPIS->CTRL.Word = ctrl;
}

bool spis_getc(uint8_t *ch)
{
    if (SPIS->STATUS.SPIS_RXFIFO_EMPTY == 0)
    {
        *ch = SPIS->RX_DAT;
        //SPIS->INFO_CLR.Word = SPIS_RXINT_CLR_BIT;
        return true;
    }
    
    return false;
}

void spis_putc(uint8_t ch)
{
    while (SPIS->STATUS.SPIS_TXFIFO_FULL);
    SPIS->TX_DAT = ch;
}
