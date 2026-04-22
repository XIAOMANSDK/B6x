#ifndef _REG_EXTI_H_
#define _REG_EXTI_H_

#include "reg_base.h" 

//================================
//BLOCK EXTI define 

#define EXTI_BASE                              ((uint32_t)0x40028000) 
#define EXTI_IER_ADDR_OFFSET                   0x000 
#define EXTI_IDR_ADDR_OFFSET                   0x004 
#define EXTI_IVS_ADDR_OFFSET                   0x008 
#define EXTI_RIF_ADDR_OFFSET                   0x00c 
#define EXTI_IFM_ADDR_OFFSET                   0x010 
#define EXTI_ICR_ADDR_OFFSET                   0x014 
#define EXTI_RTS_ADDR_OFFSET                   0x018 
#define EXTI_FTS_ADDR_OFFSET                   0x01c 
#define EXTI_SWI_ADDR_OFFSET                   0x020 
#define EXTI_ADTE_ADDR_OFFSET                  0x024 
#define EXTI_DB_ADDR_OFFSET                    0x028 
#define EXTI_DBC_ADDR_OFFSET                   0x02c 

//================================
//BLOCK EXTI reg struct define 
typedef union //0x000 
{
    struct
    {
        uint32_t EXTI_SRC0:                          1; // bit0
        uint32_t EXTI_SRC1:                          1; // bit1
        uint32_t EXTI_SRC2:                          1; // bit2
        uint32_t EXTI_SRC3:                          1; // bit3
        uint32_t EXTI_SRC4:                          1; // bit4
        uint32_t EXTI_SRC5:                          1; // bit5
        uint32_t EXTI_SRC6:                          1; // bit6
        uint32_t EXTI_SRC7:                          1; // bit7
        uint32_t EXTI_SRC8:                          1; // bit8
        uint32_t EXTI_SRC9:                          1; // bit9
        uint32_t EXTI_SRC10:                         1; // bit10
        uint32_t EXTI_SRC11:                         1; // bit11
        uint32_t EXTI_SRC12:                         1; // bit12
        uint32_t EXTI_SRC13:                         1; // bit13
        uint32_t EXTI_SRC14:                         1; // bit14
        uint32_t EXTI_SRC15:                         1; // bit15
        uint32_t EXTI_SRC16:                         1; // bit16
        uint32_t EXTI_SRC17:                         1; // bit17
        uint32_t EXTI_SRC18:                         1; // bit18
        uint32_t EXTI_SRC19:                         1; // bit19
        uint32_t RSV_END:                           12; // bit[31:20]
    };
    uint32_t Word;
} EXTI_LOCA_TypeDef; //0x000 

//================================
#define EXTI_LOCAL_SRC0_POS                  0
#define EXTI_LOCAL_SRC1_POS                  1
#define EXTI_LOCAL_SRC2_POS                  2
#define EXTI_LOCAL_SRC3_POS                  3
#define EXTI_LOCAL_SRC4_POS                  4
#define EXTI_LOCAL_SRC5_POS                  5
#define EXTI_LOCAL_SRC6_POS                  6
#define EXTI_LOCAL_SRC7_POS                  7
#define EXTI_LOCAL_SRC8_POS                  8
#define EXTI_LOCAL_SRC9_POS                  9
#define EXTI_LOCAL_SRC10_POS                 10
#define EXTI_LOCAL_SRC11_POS                 11
#define EXTI_LOCAL_SRC12_POS                 12
#define EXTI_LOCAL_SRC13_POS                 13
#define EXTI_LOCAL_SRC14_POS                 14
#define EXTI_LOCAL_SRC15_POS                 15
#define EXTI_LOCAL_SRC16_POS                 16
#define EXTI_LOCAL_SRC17_POS                 17
#define EXTI_LOCAL_SRC18_POS                 18
#define EXTI_LOCAL_SRC19_POS                 19
//================================

typedef union //0x02c 
{
    struct
    {
        uint32_t DB_CNT:                             3; // bit[2:0] --- debounce cnts,  db_cnt = db_cnt_val + 1
        uint32_t RSV_NOUSE1:                         5; // bit[7:3] --- Ignore me
        uint32_t DB_PRE:                             8; // bit[15:8]--- debounce one cnt include current clk cycles, db_pre = db_pre_val + 1
        uint32_t RSV_END:                           16; // bit[31:16]
    };
    uint32_t Word;
} EXTI_DBC_TypeDef; //0x02c 



//================================
#define EXTI_DB_CNT_LSB                     0
#define EXTI_DB_CNT_WIDTH                   3
#define EXTI_DB_PRE_LSB                     8
#define EXTI_DB_PRE_WIDTH                   8
//================================

//================================
//BLOCK EXTI top struct define 
typedef struct
{
    __O   EXTI_LOCA_TypeDef                      IER;  // 0x000, 
                                                       // exti interrupt enable register 
    __O   EXTI_LOCA_TypeDef                      IDR;  // 0x004, 
                                                       // exti interrupt disable register 
    __I   EXTI_LOCA_TypeDef                      IVS;  // 0x008, 
                                                       // exti interrupt valid status register 
    __I   EXTI_LOCA_TypeDef                      RIF;  // 0x00c, 
                                                       // exti raw interrupt flag status register 
    __I   EXTI_LOCA_TypeDef                      IFM;  // 0x010, 
                                                       // exti interrupt flag masked status register 
    __O   EXTI_LOCA_TypeDef                      ICR;  // 0x014, 
                                                       // exti interrupt clear register 
    __IO  EXTI_LOCA_TypeDef                      RTS;  // 0x018, 
                                                       // exti rising edge trigger selection register 
    __IO  EXTI_LOCA_TypeDef                      FTS;  // 0x01c, 
                                                       // exti falling edge trigger selection register 
    __IO  EXTI_LOCA_TypeDef                      SWI;  // 0x020, 
                                                       // exti software interrupt event register 
    __IO  EXTI_LOCA_TypeDef                      ADTE; // 0x024, 
                                                       // exti ad trigger enable 
    __IO  EXTI_LOCA_TypeDef                      DB;   // 0x028, 
                                                       // exti input debounce enable register 
    __IO  EXTI_DBC_TypeDef                       DBC;  // 0x02c, 
                                                       // exti input debounce sample rate control register 
} EXTI_TypeDef;


#define EXTI  (( EXTI_TypeDef  *)     EXTI_BASE)

#endif
