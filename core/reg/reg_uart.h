#ifndef _REG_UART_H_
#define _REG_UART_H_

#include "reg_base.h"

//================================
//BLOCK UART define 

#define UART1_BASE                             ((uint32_t)0x40023000) 
#define UART2_BASE                             ((uint32_t)0x40024000) 
#define UART_RBR_ADDR_OFFSET                   0x000 
#define UART_TBR_ADDR_OFFSET                   0x004 
#define UART_BRR_ADDR_OFFSET                   0x008 
#define UART_LCR_ADDR_OFFSET                   0x00c 
#define UART_MCR_ADDR_OFFSET                   0x010 
#define UART_CR_ADDR_OFFSET                    0x014 
#define UART_RTOR_ADDR_OFFSET                  0x018 
#define UART_FCR_ADDR_OFFSET                   0x01c 
#define UART_SR_ADDR_OFFSET                    0x020 
#define UART_IER_ADDR_OFFSET                   0x024 
#define UART_IDR_ADDR_OFFSET                   0x028 
#define UART_IVS_ADDR_OFFSET                   0x02c 
#define UART_RIF_ADDR_OFFSET                   0x030 
#define UART_IFM_ADDR_OFFSET                   0x034 
#define UART_ICR_ADDR_OFFSET                   0x038 

//================================
//BLOCK UART reg struct define 
typedef union //0x00c 
{
    struct
    {
        uint32_t DLS:                                2; // bit[1:0] --- data length select
        uint32_t STOP:                               1; // bit2     --- stop bit select
        uint32_t PE:                                 1; // bit3     --- parity enable bit
        uint32_t PS:                                 1; // bit4     --- parity mode select
        uint32_t RXEN:                               1; // bit5     --- uart receive enable
        uint32_t BC:                                 1; // bit6     --- break control
        uint32_t BRWEN:                              1; // bit7     --- baud rate write enable
        uint32_t RTOEN:                              1; // bit8     --- receive time out enable
        uint32_t MSBFIRST:                           1; // bit9     --- the most significant bit first
        uint32_t DATAINV:                            1; // bit10    --- binary data inversion
        uint32_t RXINV:                              1; // bit11    --- rx pin active level inversion
        uint32_t TXINV:                              1; // bit12    --- tx pin active level inversion
        uint32_t SWAP:                               1; // bit13    --- swap tx/rx pin
        uint32_t RTO_SEL:                            1; // bit14    --- receiver timeout mode select: 0:CPU; 1:DMA
        uint32_t RSV_END:                           17; // bit[31:15]
    };
    uint32_t Word;
} UART_LCR_TypeDef; //0x00c 


//================================
#define UART_DLS_LSB                        0
#define UART_DLS_WIDTH                      2
#define UART_STOP_POS                       2
#define UART_PE_POS                         3
#define UART_PS_POS                         4
#define UART_RXEN_POS                       5
#define UART_BC_POS                         6
#define UART_BRWEN_POS                      7
#define UART_RTOEN_POS                      8
#define UART_MSBFIRST_POS                   9
#define UART_DATAINV_POS                    10
#define UART_RXINV_POS                      11
#define UART_TXINV_POS                      12
#define UART_SWAP_POS                       13
#define UART_RTO_SEL_POS                    14
//================================

typedef union //0x010 
{
    struct
    {
        uint32_t IREN:                               1; // bit0       --- irDA mode enable (only uart)
        uint32_t LBEN:                               1; // bit1       --- loopback enable
        uint32_t AFCEN:                              1; // bit2       --- auto flow control enable
        uint32_t RTSCTRL:                            1; // bit3       --- auto flow RTS
        uint32_t AADEN:                              1; // bit4       --- 485 auto address en
        uint32_t AADNOR:                             1; // bit5       --- 485 auto address normal
        uint32_t AADDIR:                             1; // bit6       --- 485 auto address direction control en
        uint32_t AADINV:                             1; // bit7       --- 485 auto address RTS invert
        uint32_t LINEN:                              1; // bit8       --- lin mode en
        uint32_t BKREQ:                              1; // bit9       --- lin break request
        uint32_t LINBDL:                             1; // bit10      --- lin break detection length
        uint32_t DMAEN:                              1; // bit11      --- DMA en
        uint32_t ABREN:                              1; // bit12      --- auto baud
        uint32_t ABRMOD:                             2; // bit[14:13] --- auto baud rate
        uint32_t ABRRS:                              1; // bit15      --- auto baud restart
        uint32_t SCEN:                               1; // bit16      --- smart en
        uint32_t SCNACK:                             1; // bit17      --- smart nack
        uint32_t SCCNT:                              3; // bit[20:18] --- smart error
        uint32_t CLKEN:                              1; // bit21      --- smart clock
        uint32_t HDSEL:                              1; // bit22      --- half-duplex seletion
        uint32_t RSV_END:                            9; // bit[31:23]
    };
    uint32_t Word;
} UART_MCR_TypeDef; //0x010 


//================================
#define UART_IREN_POS                       0
#define UART_LBEN_POS                       1
#define UART_AFCEN_POS                      2
#define UART_RTSCTRL_POS                    3
#define UART_AADEN_POS                      4
#define UART_AADNOR_POS                     5
#define UART_AADDIR_POS                     6
#define UART_AADINV_POS                     7
#define UART_LINEN_POS                      8
#define UART_BKREQ_POS                      9
#define UART_LINBDL_POS                     10
#define UART_DMAEN_POS                      11
#define UART_ABREN_POS                      12
#define UART_ABRMOD_LSB                     13
#define UART_ABRMOD_WIDTH                   2
#define UART_ABRRS_POS                      15
#define UART_SCEN_POS                       16
#define UART_SCNACK_POS                     17
#define UART_SCCNT_LSB                      18
#define UART_SCCNT_WIDTH                    3
#define UART_CLKEN_POS                      21
#define UART_HDSEL_POS                      22
//================================

typedef union //0x014 
{
    struct
    {
        uint32_t ADDR:                               8; // bit[7:0]   --- rs485 address match value
        uint32_t DLY:                                8; // bit[15:8]  --- rs485 delay value
        uint32_t PSC:                                8; // bit[23:16] --- prescaler value (only uart)
        uint32_t GT:                                 8; // bit[31:24] --- guard time value (only uart)       
    };
    uint32_t Word;
} UART_CR_TypeDef; //0x014 


//================================
#define UART_ADDR_LSB                       0
#define UART_ADDR_WIDTH                     8
#define UART_DLY_LSB                        8
#define UART_DLY_WIDTH                      8
#define UART_PSC_LSB                        16
#define UART_PSC_WIDTH                      8
#define UART_GT_LSB                         24
#define UART_GT_WIDTH                       8
//================================

typedef union //0x018 
{
    struct
    {
        uint32_t RTO:                               24; // bit[23:0]  --- receive timeout value 
        uint32_t BLEN:                               8; // bit[31:24] --- block length
    };
    uint32_t Word;
} UART_RTOR_TypeDef; //0x018 


//================================
#define UART_RTO_LSB                        0
#define UART_RTO_WIDTH                      24
#define UART_BLEN_LSB                       24
#define UART_BLEN_WIDTH                     8
//================================

typedef union //0x01c 
{
    struct
    {
        uint32_t FIFOEN:                             1; // bit0       --- fifo enable
        uint32_t RFRST:                              1; // bit1       --- fifo rx reset data
        uint32_t TFRST:                              1; // bit2       --- fifo tx reset data
        uint32_t RSV_NOUSE1:                         1; // bit3       --- Ignore me
        uint32_t RXTL:                               2; // bit[5:4]   --- fifo rx trigger level
        uint32_t TXTL:                               2; // bit[7:6]   --- fifo tx trigger level
        uint32_t RXFL:                               5; // bit[12:8]  --- fifo rx level counter
        uint32_t TXFL:                               5; // bit[17:13] --- fifo tx level counter
        uint32_t RSV_END:                           14; // bit[31:18]
    };
    uint32_t Word;
} UART_FCR_TypeDef; //0x01c 


//================================
#define UART_FIFOEN_POS                     0
#define UART_RFRST_POS                      1
#define UART_TFRST_POS                      2
#define UART_RXTL_LSB                       4
#define UART_RXTL_WIDTH                     2
#define UART_TXTL_LSB                       6
#define UART_TXTL_WIDTH                     2
#define UART_RXFL_LSB                       8
#define UART_RXFL_WIDTH                     5
#define UART_TXFL_LSB                       13
#define UART_TXFL_WIDTH                     5
//================================

typedef union //0x020 
{
    struct
    {
        uint32_t DR:                                 1; // bit0  --- data ready
        uint32_t OE:                                 1; // bit1  --- overrun error
        uint32_t PE:                                 1; // bit2  --- parity error
        uint32_t FE:                                 1; // bit3  --- framing error
        uint32_t BF:                                 1; // bit4  --- break interrupt
        uint32_t TBEM:                               1; // bit5  --- transmit buffer register empty
        uint32_t TEM:                                1; // bit6  --- transmitter empty
        uint32_t RFE:                                1; // bit7  --- receiver fifo data error
        uint32_t BUSY:                               1; // bit8  --- uart busy
        uint32_t TFNF:                               1; // bit9  --- transmit fifo not full
        uint32_t TFEM:                               1; // bit10 --- transmit fifo empty
        uint32_t RFNE:                               1; // bit11 --- receive fifo not empty
        uint32_t RFF:                                1; // bit12 --- receive fifo full
        uint32_t DCTS:                               1; // bit13 --- delta clear to send
        uint32_t CTS:                                1; // bit14 --- clear to send
        uint32_t RSV_END:                           17; // bit[31:15]
    };
    uint32_t Word;
} UART_SR_TypeDef; //0x020 


//================================
#define UART_SR_DR_POS                         0
#define UART_SR_OE_POS                         1
#define UART_SR_PE_POS                         2
#define UART_SR_FE_POS                         3
#define UART_SR_BF_POS                         4
#define UART_SR_TBEM_POS                       5
#define UART_SR_TEM_POS                        6
#define UART_SR_RFE_POS                        7
#define UART_SR_BUSY_POS                       8
#define UART_SR_TFNF_POS                       9
#define UART_SR_TFEM_POS                       10
#define UART_SR_RFNE_POS                       11
#define UART_SR_RFF_POS                        12
#define UART_SR_DCTS_POS                       13
#define UART_SR_CTS_POS                        14
//================================

typedef union //0x024 
{
    struct
    {
        uint32_t RXRD:                               1; // bit0  --- receive data available interrupt enable status
        uint32_t TXS:                                1; // bit1  --- thr empty interrupt enable status
        uint32_t RXS:                                1; // bit2  --- receiver line status interrupt enable status
        uint32_t MDS:                                1; // bit3  --- modem status interrupt enable status
        uint32_t RTO:                                1; // bit4  --- receiver timeout enable status
        uint32_t BUSY:                               1; // bit5  --- busy detect interrupt enable status
        uint32_t ABE:                                1; // bit6  --- auto baud rate end interrupt enable status
        uint32_t ABTO:                               1; // bit7  --- auto baud rate timeout interrupt enable status
        uint32_t LINBK:                              1; // bit8  --- lin break detection interrupt enable status
        uint32_t TC:                                 1; // bit9  --- transmission complete interrupt enable status
        uint32_t EOB:                                1; // bit10 --- end of block interrupt enable status
        uint32_t CM:                                 1; // bit11 --- character match interrupt enable status
        uint32_t RSV_END:                           20; // bit[31:12]
    };
    uint32_t Word;
} UART_IER_TypeDef; //0x024 


//================================
#define UART_INT_RXRD_POS                       0
#define UART_INT_TXS_POS                        1
#define UART_INT_RXS_POS                        2
#define UART_INT_MDS_POS                        3
#define UART_INT_RTO_POS                        4
#define UART_INT_BUSY_POS                       5
#define UART_INT_ABE_POS                        6
#define UART_INT_ABTO_POS                       7
#define UART_INT_LINBK_POS                      8
#define UART_INT_TC_POS                         9
#define UART_INT_EOB_POS                        10
#define UART_INT_CM_POS                         11
//================================

typedef union //0x028 
{
    struct
    {
        uint32_t RXRD:                               1; // bit0  --- receive data available interrupt enable status
        uint32_t TXS:                                1; // bit1  --- thr empty interrupt enable status
        uint32_t RXS:                                1; // bit2  --- receiver line status interrupt enable status
        uint32_t MDS:                                1; // bit3  --- modem status interrupt enable status
        uint32_t RTO:                                1; // bit4  --- receiver timeout enable status
        uint32_t BUSY:                               1; // bit5  --- busy detect interrupt enable status
        uint32_t ABE:                                1; // bit6  --- auto baud rate end interrupt enable status
        uint32_t ABTO:                               1; // bit7  --- auto baud rate timeout interrupt enable status
        uint32_t LINBK:                              1; // bit8  --- lin break detection interrupt enable status
        uint32_t TC:                                 1; // bit9  --- transmission complete interrupt enable status
        uint32_t EOB:                                1; // bit10 --- end of block interrupt enable status
        uint32_t CM:                                 1; // bit11 --- character match interrupt enable status
        uint32_t RSV_END:                           20; // bit[31:12]
    };
    uint32_t Word;
} UART_IDR_TypeDef; //0x028 

typedef union //0x02c 
{
    struct
    {
        uint32_t RXRD:                               1; // bit0  --- receive data available interrupt enable status
        uint32_t TXS:                                1; // bit1  --- thr empty interrupt enable status
        uint32_t RXS:                                1; // bit2  --- receiver line status interrupt enable status
        uint32_t MDS:                                1; // bit3  --- modem status interrupt enable status
        uint32_t RTO:                                1; // bit4  --- receiver timeout enable status
        uint32_t BUSY:                               1; // bit5  --- busy detect interrupt enable status
        uint32_t ABE:                                1; // bit6  --- auto baud rate end interrupt enable status
        uint32_t ABTO:                               1; // bit7  --- auto baud rate timeout interrupt enable status
        uint32_t LINBK:                              1; // bit8  --- lin break detection interrupt enable status
        uint32_t TC:                                 1; // bit9  --- transmission complete interrupt enable status
        uint32_t EOB:                                1; // bit10 --- end of block interrupt enable status
        uint32_t CM:                                 1; // bit11 --- character match interrupt enable status
        uint32_t RSV_END:                           20; // bit[31:12]
    };
    uint32_t Word;
} UART_IVS_TypeDef; //0x02c 

typedef union //0x030 
{
    struct
    {
        uint32_t RXRD:                               1; // bit0  --- receive data available interrupt enable status
        uint32_t TXS:                                1; // bit1  --- thr empty interrupt enable status
        uint32_t RXS:                                1; // bit2  --- receiver line status interrupt enable status
        uint32_t MDS:                                1; // bit3  --- modem status interrupt enable status
        uint32_t RTO:                                1; // bit4  --- receiver timeout enable status
        uint32_t BUSY:                               1; // bit5  --- busy detect interrupt enable status
        uint32_t ABE:                                1; // bit6  --- auto baud rate end interrupt enable status
        uint32_t ABTO:                               1; // bit7  --- auto baud rate timeout interrupt enable status
        uint32_t LINBK:                              1; // bit8  --- lin break detection interrupt enable status
        uint32_t TC:                                 1; // bit9  --- transmission complete interrupt enable status
        uint32_t EOB:                                1; // bit10 --- end of block interrupt enable status
        uint32_t CM:                                 1; // bit11 --- character match interrupt enable status
        uint32_t RSV_END:                           20; // bit[31:12]
    };
    uint32_t Word;
} UART_RIF_TypeDef; //0x030 

typedef union //0x034 
{
    struct
    {
        uint32_t RXRD:                               1; // bit0  --- receive data available interrupt enable status
        uint32_t TXS:                                1; // bit1  --- thr empty interrupt enable status
        uint32_t RXS:                                1; // bit2  --- receiver line status interrupt enable status
        uint32_t MDS:                                1; // bit3  --- modem status interrupt enable status
        uint32_t RTO:                                1; // bit4  --- receiver timeout enable status
        uint32_t BUSY:                               1; // bit5  --- busy detect interrupt enable status
        uint32_t ABE:                                1; // bit6  --- auto baud rate end interrupt enable status
        uint32_t ABTO:                               1; // bit7  --- auto baud rate timeout interrupt enable status
        uint32_t LINBK:                              1; // bit8  --- lin break detection interrupt enable status
        uint32_t TC:                                 1; // bit9  --- transmission complete interrupt enable status
        uint32_t EOB:                                1; // bit10 --- end of block interrupt enable status
        uint32_t CM:                                 1; // bit11 --- character match interrupt enable status
        uint32_t RSV_END:                           20; // bit[31:12]
    };
    uint32_t Word;
} UART_IFM_TypeDef; //0x034 

typedef union //0x038 
{
    struct
    {
        uint32_t RXRD:                               1; // bit0  --- receive data available interrupt enable status
        uint32_t TXS:                                1; // bit1  --- thr empty interrupt enable status
        uint32_t RXS:                                1; // bit2  --- receiver line status interrupt enable status
        uint32_t MDS:                                1; // bit3  --- modem status interrupt enable status
        uint32_t RTO:                                1; // bit4  --- receiver timeout enable status
        uint32_t BUSY:                               1; // bit5  --- busy detect interrupt enable status
        uint32_t ABE:                                1; // bit6  --- auto baud rate end interrupt enable status
        uint32_t ABTO:                               1; // bit7  --- auto baud rate timeout interrupt enable status
        uint32_t LINBK:                              1; // bit8  --- lin break detection interrupt enable status
        uint32_t TC:                                 1; // bit9  --- transmission complete interrupt enable status
        uint32_t EOB:                                1; // bit10 --- end of block interrupt enable status
        uint32_t CM:                                 1; // bit11 --- character match interrupt enable status
        uint32_t RSV_END:                           20; // bit[31:12]
    };
    uint32_t Word;
} UART_ICR_TypeDef; //0x038 


//================================
//BLOCK UART top struct define 
typedef struct
{
    __IO  uint32_t                               RBR ; // 0x000, 
                                                       // Receive  buffer register 
    __IO  uint32_t                               TBR ; // 0x004, 
                                                       // Transmit buffer register 
    __IO  uint32_t                               BRR ; // 0x008, 
                                                       // Baud rate register 
    __IO  UART_LCR_TypeDef                       LCR ; // 0x00c, 
                                                       // Line  control register 
    __IO  UART_MCR_TypeDef                       MCR ; // 0x010, 
                                                       // Mode  control register 
    __IO  UART_CR_TypeDef                        CR  ; // 0x014, 
                                                       // Control register 
    __IO  UART_RTOR_TypeDef                      RTOR; // 0x018, 
                                                       // Receiver timeout register 
    __IO  UART_FCR_TypeDef                       FCR ; // 0x01c, 
                                                       // FIFO  control register 
    __IO  UART_SR_TypeDef                        SR  ; // 0x020, 
                                                       // Status register 
    __O   UART_IER_TypeDef                       IER ; // 0x024, 
                                                       // Interrupt enable  register 
    __O   UART_IDR_TypeDef                       IDR ; // 0x028, 
                                                       // Interrupt disable register 
    __I   UART_IVS_TypeDef                       IVS ; // 0x02c, 
                                                       // Interrupt valid status 
    __I   UART_RIF_TypeDef                       RIF ; // 0x030, 
                                                       // Raw interrupt flag 
    __I   UART_IFM_TypeDef                       IFM ; // 0x034, 
                                                       // Interrupt flag masked 
    __O   UART_ICR_TypeDef                       ICR ; // 0x038, 
                                                       // Interrupt clear register 
} UART_TypeDef;


#define UART1  (( UART_TypeDef  *)     UART1_BASE)
#define UART2  (( UART_TypeDef  *)     UART2_BASE)

#endif
