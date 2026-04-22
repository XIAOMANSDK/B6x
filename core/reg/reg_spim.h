#ifndef _REG_SPIM_H_
#define _REG_SPIM_H_

#include "reg_base.h" 

//================================
//BLOCK SPIM define 

#define SPIM_BASE                              ((uint32_t)0x40004000) 
#define SPIM_RX_DATA_ADDR_OFFSET               0x000 
#define SPIM_TX_DATA_ADDR_OFFSET               0x004 
#define SPIM_TXRX_BGN_ADDR_OFFSET              0x008 
#define SPIM_CTRL_ADDR_OFFSET                  0x00c 
#define SPIM_STATUS_CLR_ADDR_OFFSET            0x010 
#define SPIM_STATUS_ADDR_OFFSET                0x014 
#define SPIM_DAT_LEN_ADDR_OFFSET               0x018 

//================================
//BLOCK SPIM reg struct define 
typedef union //0x00c 
{
    struct
    {
        uint32_t SPIM_CRAT:                          4; // bit[3:0] --- clock rate select,clk/2^(crat+1)
                                                        // 0-11 is supported
        uint32_t SPIM_CPHA:                          1; // bit4 --- clock phase bit
                                                        // 0: sck sample data in first edge;
                                                        // 1: sck sample data in second edge;
        uint32_t SPIM_CPOL:                          1; // bit5 --- clock polarity bit
                                                        // 0: sck is low in idle status;
                                                        // 1: sck is high in idle status;
        uint32_t SPIM_TX_DMA_EN:                     1; // bit6 ---
                                                        // 0: TX work as mcu mode
                                                        // 1: TX work as dma mode,after configure dma cfg,set txrx bgn to 1 to start the dma transission
        uint32_t SPIM_RX_DMA_EN:                     1; // bit7 ---
                                                        // 0: RX work as mcu mode
                                                        // 1: RX work as dma mode,after configure dma cfg,set txrx bgn to 1 to start the dma transission
        uint32_t SPIM_INT_EN:                        1; // bit8 --- interrupt enable bit
        uint32_t SPIM_TX_EN:                         1; // bit9 --- transmit to SPI slave enable
        uint32_t SPIM_RX_EN:                         1; // bit10--- receive from SPI slave enable
        uint32_t SPIM_MSB_FST:                       1; // bit11---
                                                        // 0: LSB first
                                                        // 1: MSB first
        uint32_t RSV_END:                           20; // bit[31:12]
    };
    uint32_t Word;
} SPIM_CTRL_TypeDef; //0x00c 


//================================
#define SPIM_SPIM_CRAT_LSB                  0
#define SPIM_SPIM_CRAT_WIDTH                4
#define SPIM_SPIM_CPHA_POS                  4
#define SPIM_SPIM_CPOL_POS                  5
#define SPIM_SPIM_TX_DMA_EN_POS             6
#define SPIM_SPIM_RX_DMA_EN_POS             7
#define SPIM_SPIM_INT_EN_POS                8
#define SPIM_SPIM_TX_EN_POS                 9
#define SPIM_SPIM_RX_EN_POS                 10
#define SPIM_SPIM_MSB_FST_POS               11
#define SPIM_SPIM_RX_PH_POS                 12
//================================

typedef union //0x010 
{
    struct
    {
        uint32_t SPIM_TXDAT_CLR:                     1; // bit0 --- set 1 to clear spi TX fifo status and inner counter
        uint32_t SPIM_RXDAT_CLR:                     1; // bit1 --- set 1 to clear spi RX fifo status and inner counter
        uint32_t SPIM_CLR_INTF:                      1; // bit2 --- write 1 to clear intf status 
        uint32_t RSV_END:                           29; // bit[31:3]
    };
    uint32_t Word;
} SPIM_STATUS_CLR_TypeDef; //0x010 


//================================
#define SPIM_SPIM_TXDAT_CLR_POS             0
#define SPIM_SPIM_RXDAT_CLR_POS             1
#define SPIM_SPIM_CLR_INTF_POS              2
//================================

typedef union //0x014 
{
    struct
    {
        uint32_t SPIM_TX_FEMPTY:                     1; // bit0     --- tx fifo empty
        uint32_t SPIM_TX_FFULL:                      1; // bit1     --- tx fifo full
        uint32_t SPIM_TX_FNUM:                       3; // bit[4:2] --- data number in tx fifo
        uint32_t SPIM_RX_FEMPTY:                     1; // bit5     --- rx fifo empty
        uint32_t SPIM_RX_FFULL:                      1; // bit6     --- rx fifo full
        uint32_t SPIM_RX_FNUM:                       3; // bit[9:7] --- data length in rx fifo
        uint32_t SPIM_INTF:                          1; // bit10    --- rx/tx interrupt flag
        uint32_t SPIM_BUSY:                          1; // bit11    --- spim transfer busy signal
                                                        // 0: spim idle
                                                        // 1: spim busy,tx/rx is running
        uint32_t RSV_END:                           20; // bit[31:12]
    };
    uint32_t Word;
} SPIM_STATUS_TypeDef; //0x014 


//================================
#define SPIM_SPIM_TX_FEMPTY_POS             0
#define SPIM_SPIM_TX_FFULL_POS              1
#define SPIM_SPIM_TX_FNUM_LSB               2
#define SPIM_SPIM_TX_FNUM_WIDTH             3
#define SPIM_SPIM_RX_FEMPTY_POS             5
#define SPIM_SPIM_RX_FFULL_POS              6
#define SPIM_SPIM_RX_FNUM_LSB               7
#define SPIM_SPIM_RX_FNUM_WIDTH             3
#define SPIM_SPIM_INTF_POS                  10
#define SPIM_SPIM_BUSY_POS                  11
//================================

//================================
//BLOCK SPIM top struct define 
typedef struct
{
    __I   uint32_t                               RX_DATA             ; // 0x000,  
    __O   uint32_t                               TX_DATA             ; // 0x004,  
    __O   uint32_t                               TXRX_BGN            ; // 0x008, 
                                                                       // used when dullplex mode or only RX mode 
    __IO  SPIM_CTRL_TypeDef                      CTRL                ; // 0x00c,  
    __O   SPIM_STATUS_CLR_TypeDef                STATUS_CLR          ; // 0x010,  
    __I   SPIM_STATUS_TypeDef                    STATUS              ; // 0x014,  
    __IO  uint32_t                               DAT_LEN             ; // 0x018, 
                                                                       // data length for RX in MCU or DMA mode
                                                                       // valid bit[15:0] 
} SPIM_TypeDef;


#define SPIM  (( SPIM_TypeDef  *)     SPIM_BASE)

#endif
