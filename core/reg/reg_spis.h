#ifndef _REG_SPIS_H_
#define _REG_SPIS_H_

#include "reg_base.h" 

//================================
//BLOCK SPIS define 

#define SPIS_BASE                              ((uint32_t)0x40005000) 
#define SPIS_CTRL_ADDR_OFFSET                  0x000 
#define SPIS_STATUS_ADDR_OFFSET                0x004 
#define SPIS_INFO_CLR_ADDR_OFFSET              0x008 
#define SPIS_TX_DAT_ADDR_OFFSET                0x00c 
#define SPIS_RX_DAT_ADDR_OFFSET                0x010 

//================================
//BLOCK SPIS reg struct define 
typedef union //0x000 
{
    struct
    {
        uint32_t SPIS_LSBFIRST:                      1; // bit0 ---
                                                        // 0: MSB first for SPIS TX/RX;
                                                        // 1: LSB first for SPIS TX/RX;
        uint32_t SPIS_CPOL:                          1; // bit1 --- spi clock polarity control bit when SPIS is IDLE(cs is 1);
                                                        // 0: sample the data at the first clock edge
                                                        // 1: sample the data at the second clock edge
        uint32_t SPIS_CPHA:                          1; // bit2 --- clock phase used for sample data
                                                        // 0: sample the data at the first clock edge
                                                        // 1: sample the data at the second clock edge
        uint32_t SPIS_RX_EN:                         1; // bit3 --- spis rx control signal
                                                        // default is enable
                                                        // 0: spis RX disable
                                                        // 1: spis RX enable
                                                        // SPIS RX enable only when both spis en and spis rx en set as 1
        uint32_t SPIS_RXINT_EN:                      1; // bit4 ---
                                                        // 0: spis_rxint interrupt disable
                                                        // 1: spis_rxint interrupt enable
        uint32_t SPIS_TX_DMA_MODE:                   1; // bit5 --- SPIS TX operation mode control signal
                                                        // 1: DMA mode
                                                        // 0: MCU mode
        uint32_t SPIS_RX_DMA_MODE:                   1; // bit6 --- SPIS RX operation mode control signal
                                                        // 1: DMA mode
                                                        // 0: MCU mode
        uint32_t SPIS_EN:                            1; // bit7 ---
                                                        // 0: SPIS work disable
                                                        // 1: spis work enable
                                                        // [Note:the spis lsfirst,spis spol and spis spha can be configured only when spis en is 0]
        uint32_t SPIS_OT_WIN:                        8; // bit[15:8] --- config overtime window
                                                        // unit : bit width time
        uint32_t SPIS_OT_EN:                         1; // bit16 --- SPIS overtime enable
        uint32_t SPIS_OTINT_EN:                      1; // bit17
        uint32_t RSV_END:                           14; // bit[31:18]
    };
    uint32_t Word;
} SPIS_CTRL_TypeDef; //0x000 


//================================
#define SPIS_SPIS_LSBFIRST_POS              0
#define SPIS_SPIS_CPOL_POS                  1
#define SPIS_SPIS_CPHA_POS                  2
#define SPIS_SPIS_RX_EN_POS                 3
#define SPIS_SPIS_RXINT_EN_POS              4
#define SPIS_SPIS_TX_DMA_MODE_POS           5
#define SPIS_SPIS_RX_DMA_MODE_POS           6
#define SPIS_SPIS_EN_POS                    7
#define SPIS_SPIS_OT_WIN_LSB                8
#define SPIS_SPIS_OT_WIN_WIDTH              8
#define SPIS_SPIS_OT_EN_POS                 16
#define SPIS_SPIS_OTINT_EN_POS              17
//================================

typedef union //0x004 
{
    struct
    {
        uint32_t SPIS_RXFIFO_EMPTY:                  1; // bit0 ---
                                                        // 0: spis_rxfifo is not empty
                                                        // 1: spis_rxfifo is empty
        uint32_t SPIS_RXFIFO_FULL:                   1; // bit1 ---
                                                        // 0: spis_rxfifo is not full
                                                        // 1: spis_rxfifo is full
        uint32_t SPIS_RXFIFO_NUM:                    3; // bit[4:2] --- the SPIS RX data number in rxfifo
        uint32_t RSV_NOUSE1:                         1; // bit5     --- Ignore me
        uint32_t SPIS_RX_OVERRUN:                    1; // bit6     --- SPIS rxfifo overrun error signal
                                                        // 0: cleared by spis rx overrun clr
                                                        // 1: rxfifo overrun error
        uint32_t SPIS_CS:                            1; // bit7     --- the signal pin SPIS_CS
                                                        // 0: SPIS is selected
                                                        // 1: SPIS is not selected
        uint32_t SPIS_TXFIFO_EMPTY:                  1; // bit8 ---
                                                        // 0: spis_txfifo is not empty
                                                        // 1: spis_txfifo is empty
        uint32_t SPIS_TXFIFO_FULL:                   1; // bit9 ---
                                                        // 0: spis_txfifo is not full
                                                        // 1: spis_txfifo is full
                                                        // [Note:When txfifo is not full,MCU can write tx dat reg
        uint32_t SPIS_TXFIFO_NUM:                    3; // bit[12:10] --- the SPIS TX data number in txfifo
        uint32_t RSV_NOUSE2:                         3; // bit[15:13] --- Ignore me
        uint32_t SPIS_RXINT_ST:                      1; // bit16      --- when SPIS RX 1 byte,spis_rxint is 1
                                                        // cleared by spis_rxint_clr
        uint32_t SPIS_OTINT_ST:                      1; // bit17      --- set as 1 when SPIS overtime event happen
        uint32_t RSV_END:                           14; // bit[31:18]
    };
    uint32_t Word;
} SPIS_STATUS_TypeDef; //0x004 


//================================
#define SPIS_SPIS_RXFIFO_EMPTY_POS          0
#define SPIS_SPIS_RXFIFO_FULL_POS           1
#define SPIS_SPIS_RXFIFO_NUM_LSB            2
#define SPIS_SPIS_RXFIFO_NUM_WIDTH          3
#define SPIS_SPIS_RX_OVERRUN_POS            6
#define SPIS_SPIS_CS_POS                    7
#define SPIS_SPIS_TXFIFO_EMPTY_POS          8
#define SPIS_SPIS_TXFIFO_FULL_POS           9
#define SPIS_SPIS_TXFIFO_NUM_LSB            10
#define SPIS_SPIS_TXFIFO_NUM_WIDTH          3
#define SPIS_SPIS_RXINT_ST_POS              16
#define SPIS_SPIS_OTINT_ST_POS              17
//================================

typedef union //0x008 
{
    struct
    {
        uint32_t SPIS_RX_OVERRUN_CLR:                1; // bit0 --- use to clear spis_rx_overrun
        uint32_t SPIS_TXDAT_CLR:                     1; // bit1 --- use to clear SPIS txfifo point and data reg
        uint32_t SPIS_RXDAT_CLR:                     1; // bit2 --- use to clear SPIS rxfifo point and data reg
        uint32_t SPIS_RXINT_CLR:                     1; // bit3 --- use clear spis_rxint
        uint32_t SPIS_OTINT_CLR:                     1; // bit4
        uint32_t RSV_END:                           27; // bit[31:5]
    };
    uint32_t Word;
} SPIS_INFO_CLR_TypeDef; //0x008 


//================================
#define SPIS_SPIS_RX_OVERRUN_CLR_POS        0
#define SPIS_SPIS_TXDAT_CLR_POS             1
#define SPIS_SPIS_RXDAT_CLR_POS             2
#define SPIS_SPIS_RXINT_CLR_POS             3
#define SPIS_SPIS_OTINT_CLR_POS             4
//================================

//================================
//BLOCK SPIS top struct define 
typedef struct
{
    __IO  SPIS_CTRL_TypeDef                      CTRL                ; // 0x000,  
    __I   SPIS_STATUS_TypeDef                    STATUS              ; // 0x004,  
    __O   SPIS_INFO_CLR_TypeDef                  INFO_CLR            ; // 0x008,  
    __O   uint32_t                               TX_DAT              ; // 0x00c,  
    __I   uint32_t                               RX_DAT              ; // 0x010,  
} SPIS_TypeDef;


#define SPIS  (( SPIS_TypeDef  *)     SPIS_BASE)

#endif
