#ifndef _REG_BTMR_H_
#define _REG_BTMR_H_

#include "reg_base.h" 

//================================
//BLOCK BTMR define 

#define BTMR_BASE                              ((uint32_t)0x40020000) 
#define BTMR_CR1_ADDR_OFFSET                   0x000 
#define BTMR_CR2_ADDR_OFFSET                   0x004 
#define BTMR_IER_ADDR_OFFSET                   0x00c 
#define BTMR_IDR_ADDR_OFFSET                   0x010 
#define BTMR_IVS_ADDR_OFFSET                   0x014 
#define BTMR_RIF_ADDR_OFFSET                   0x018 
#define BTMR_IFM_ADDR_OFFSET                   0x01c 
#define BTMR_ICR_ADDR_OFFSET                   0x020 
#define BTMR_EGR_ADDR_OFFSET                   0x024 
#define BTMR_CNT_ADDR_OFFSET                   0x034 
#define BTMR_PSC_ADDR_OFFSET                   0x038 
#define BTMR_ARR_ADDR_OFFSET                   0x03c 
#define BTMR_DMAEN_ADDR_OFFSET                 0x058 

//================================
//BLOCK BTMR reg struct define 
typedef union //0x000 
{
    struct
    {
        uint32_t CEN:                                1; // bit0 --- counter enable 
        uint32_t UDIS:                               1; // bit1 --- update disable
        uint32_t URS:                                1; // bit2 --- update request source
        uint32_t OPM:                                1; // bit3 --- one plus mode
        uint32_t RSV_NOUSE1:                         3; // bit[6:4]-Ignore me
        uint32_t ARPE:                               1; // bit7 --- auto reload preload enable
        uint32_t RSV_END:                           24; // bit[31:8]
    };
    uint32_t Word;
} BTMR_CR1_TypeDef; //0x000 


//================================
#define BTMR_CEN_POS                        0
#define BTMR_UDIS_POS                       1
#define BTMR_URS_POS                        2
#define BTMR_OPM_POS                        3
#define BTMR_ARPE_POS                       7
//================================

typedef union //0x004 
{
    struct
    {
        uint32_t RSV_NOUSE1:                         4; // bit[3:0] --- Ignore me
        uint32_t MMS:                                3; // bit[6:4] --- master mode select=
        uint32_t RSV_END:                           25; // bit[31:7]
    };
    uint32_t Word;
} BTMR_CR2_TypeDef; //0x004 


//================================
#define BTMR_MMS_LSB                        4
#define BTMR_MMS_WIDTH                      3
//================================

//================================
//BLOCK BTMR top struct define 
typedef struct
{
    __IO  BTMR_CR1_TypeDef                       CR1                 ; // 0x000, 
                                                                       // control register 1 
    __IO  BTMR_CR2_TypeDef                       CR2                 ; // 0x004, 
                                                                       // control register 2 
    __I   uint32_t                               RSV0[1]             ;
    __O   uint32_t                               IER                 ; // 0x00c, 
                                                                       // interrupt enable register 
    __O   uint32_t                               IDR                 ; // 0x010, 
                                                                       // interrupt disable register 
    __I   uint32_t                               IVS                 ; // 0x014, 
                                                                       // interrupt vaild status register 
    __I   uint32_t                               RIF                 ; // 0x018, 
                                                                       // interrupt raw flag register 
    __I   uint32_t                               IFM                 ; // 0x01c, 
                                                                       // interrupt masked flag register 
    __O   uint32_t                               ICR                 ; // 0x020, 
                                                                       // interrupt clear status register 
    __IO  uint32_t                               EGR                 ; // 0x024, 
                                                                       // event generation register 
    __I   uint32_t                               RSV1[3]             ;
    __IO  uint32_t                               CNT                 ; // 0x034, 
                                                                       // timer counter 
    __IO  uint32_t                               PSC                 ; // 0x038, 
                                                                       // prescaler 
    __IO  uint32_t                               ARR                 ; // 0x03c, 
                                                                       // auto-reload register 
    __I   uint32_t                               RSV2[6]             ;
    __IO  uint32_t                               DMAEN               ; // 0x058, 
                                                                       // dma trigger event enable 
} BTMR_TypeDef;


#define BTMR  (( BTMR_TypeDef  *)     BTMR_BASE)

#endif
