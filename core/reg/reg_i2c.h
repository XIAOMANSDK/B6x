#ifndef _REG_I2C_H_
#define _REG_I2C_H_

#include "reg_base.h" 

//================================
//BLOCK I2C define 

#define I2C_BASE                               ((uint32_t)0x40029000) 
#define I2C_CONSET_ADDR_OFFSET                 0x000 
#define I2C_CONCLR_ADDR_OFFSET                 0x004 
#define I2C_SR_ADDR_OFFSET                     0x008 
#define I2C_DATA_ADDR_OFFSET                   0x00c 
#define I2C_CLK_ADDR_OFFSET                    0x010 
#define I2C_SADR0_ADDR_OFFSET                  0x014 
#define I2C_ADM0_ADDR_OFFSET                   0x018 
#define I2C_XSADR_ADDR_OFFSET                  0x01c 
#define I2C_XADM_ADDR_OFFSET                   0x020 
#define I2C_SRST_ADDR_OFFSET                   0x024 
#define I2C_SADR1_ADDR_OFFSET                  0x028 
#define I2C_ADM1_ADDR_OFFSET                   0x02c 
#define I2C_SADR2_ADDR_OFFSET                  0x030 
#define I2C_ADM2_ADDR_OFFSET                   0x034 
#define I2C_SADR3_ADDR_OFFSET                  0x038 
#define I2C_ADM3_ADDR_OFFSET                   0x03c 

//================================
//BLOCK I2C reg struct define 
typedef union //0x000 
{
    struct
    {
        uint32_t SL7VAL:                             1; // bit0 --- 7-bit addr control bit
        uint32_t SL10VAL:                            1; // bit1 --- 10-bit addr control bit
        uint32_t AAK:                                1; // bit2 --- 
                                                        // 1.when this bit is 1 and satisfy any of the following conditions
                                                        //  (1)rcv 7-bit addr first byte or 10-bit second byte 
                                                        //  (2)rcv radio addr and gcebab is 1
                                                        //  (3)master or slaver mode receive data
                                                        // i2c will transmit a ack(sda low level)
                                                        // 2.if this bit is 0, when master or slaver mode receive data will not transmit ack
                                                        // 3.when slaver mode transmit data, clear this bit,slaver think this data is last
        uint32_t IFLG:                               1; // bit3 --- flag signal
        uint32_t STP:                                1; // bit4 ---  master stop  signal auto clear
                                                        //  master mode set stp 1 will send stop signal
                                                        //  slaver mode set stp 1, slaver think receive stop signal
        uint32_t STA:                                1; // bit5 ---  master start signal auto clear 
                                                        // master mode set sta 1, change into master and send start signal if already master mode, send restart signal
                                                        // slaver mode set sta 1, after ending current tasks then change into master mode
        uint32_t ENAB:                               1; // bit6 --- i2c enable
        uint32_t IEN:                                1; // bit7 --- i2c interrupt enable
        uint32_t GCAVAL:                             1; // bit8
        uint32_t RSV_END:                           23; // bit[31:9]
    };
    uint32_t Word;
} I2C_CONSET_TypeDef; //0x000 


//================================
#define I2C_SL7VAL_POS                      0
#define I2C_SL10VAL_POS                     1
#define I2C_AAK_POS                         2
#define I2C_IFLG_POS                        3
#define I2C_STP_POS                         4
#define I2C_STA_POS                         5
#define I2C_ENAB_POS                        6
#define I2C_IEN_POS                         7
#define I2C_GCAVAL_POS                      8
//================================

typedef union //0x004 
{
    struct
    {
        uint32_t RSV_NOUSE1:                         2; // bit[1:0] --- Ignore me
        uint32_t AAC:                                1; // bit2
        uint32_t IFLG:                               1; // bit3
        uint32_t RSV_NOUSE2:                         1; // bit4 --- Ignore me
        uint32_t STAC:                               1; // bit5
        uint32_t ENC:                                1; // bit6
        uint32_t IEC:                                1; // bit7
        uint32_t RSV_END:                           24; // bit[31:8]
    };
    uint32_t Word;
} I2C_CONCLR_TypeDef; //0x004 


//================================
#define I2C_AAC_POS                         2
#define I2C_IFLG_POS                        3
#define I2C_STAC_POS                        5
#define I2C_ENC_POS                         6
#define I2C_IEC_POS                         7
//================================

typedef union //0x008 
{
    struct
    {
        uint32_t RSV_NOUSE1:                         3; // bit[2:0] --- Ignore me
        uint32_t STATUS:                             5; // bit[7:3] --- i2c_status
        uint32_t RSV_END:                           24; // bit[31:8]
    };
    uint32_t Word;
} I2C_SR_TypeDef; //0x008 


//================================
#define I2C_STATUS_LSB                      3
#define I2C_STATUS_WIDTH                    5
//================================

typedef union //0x00c 
{
    struct
    {
        uint32_t DATA:                               8; // bit[7:0] --- transfer address/data
        uint32_t REV_END:                           24; // bit[31:7]
    };
    uint32_t Word;
} I2C_DATA_TypeDef; //0x00c 


//================================
#define I2C_DATA_LSB                        0
#define I2C_DATA_WIDTH                      8
#define I2C_REV_END_LSB                     8
#define I2C_REV_END_WIDTH                   24
//================================

typedef union //0x010 
{
    struct
    {
        uint32_t M:                                  4; // bit[3:0] --- i2c master mode scl frequency   
        uint32_t N:                                  3; // bit[6:4] --- i2c samp frequency 
        uint32_t RSV_END:                           25; // bit[31:7]
    };
    uint32_t Word;
} I2C_CLK_TypeDef; //0x010 


//================================
#define I2C_M_LSB                           0
#define I2C_M_WIDTH                         4
#define I2C_N_LSB                           4
#define I2C_N_WIDTH                         3
//================================

typedef union //0x014 
{
    struct
    {
        uint32_t GCEBAB:                             1; // bit0
        uint32_t SADR:                               7; // bit[7:1] --- slaa0 
        uint32_t RSV_END:                           24; // bit[31:8]
    };
    uint32_t Word;
} I2C_SADR0_TypeDef; //0x014 


//================================
#define I2C_GCEBAB_POS                      0
#define I2C_SADR_LSB                        1
#define I2C_SADR_WIDTH                      7
//================================

typedef union //0x018 
{
    struct
    {
        uint32_t RSV_NOUSE1:                         1; // bit0
        uint32_t MASK:                               7; // bit[7:1] --- adm0
        uint32_t RSV_END:                           24; // bit[31:8]
    };
    uint32_t Word;
} I2C_ADM0_TypeDef; //0x018 


//================================
#define I2C_MASK_LSB                        1
#define I2C_MASK_WIDTH                      7
//================================

typedef union //0x01c 
{
    struct
    {
        uint32_t XGCENAB:                            1; // bit0
        uint32_t ADDR:                              10; // bit[10:1] --- xsla
        uint32_t RSV_END:                           21; // bit[31:11]
    };
    uint32_t Word;
} I2C_XSADR_TypeDef; //0x01c 


//================================
#define I2C_XGCENAB_POS                     0
#define I2C_ADDR_LSB                        1
#define I2C_ADDR_WIDTH                      10
//================================

typedef union //0x020 
{
    struct
    {
        uint32_t RSV_NOUSE1:                         1; // bit0 --- Ignore me
        uint32_t MASK:                              10; // bit[10:1]
        uint32_t RSV_END:                           21; // bit[31:11]
    };
    uint32_t Word;
} I2C_XADM_TypeDef; //0x020 


//================================
#define I2C_XADM_MASK_LSB                    1
#define I2C_XADM_MASK_WIDTH                  10
//================================

typedef union //0x028 
{
    struct
    {
        uint32_t GCEBAB:                             1; // bit0
        uint32_t SADR:                               7; // bit[7:1] --- slaa1 
        uint32_t RSV_END:                           24; // bit[31:8]
    };
    uint32_t Word;
} I2C_SADR1_TypeDef; //0x028 


//================================
#define I2C_GCEBAB_POS                      0
#define I2C_SADR_LSB                        1
#define I2C_SADR_WIDTH                      7
//================================

typedef union //0x02c 
{
    struct
    {
        uint32_t RSV_NOUSE1:                         1; // bit0 --- Ignore me
        uint32_t MASK:                               7; // bit[7:1] --- adm1
        uint32_t RSV_END:                           24; // bit[31:8]
    };
    uint32_t Word;
} I2C_ADM1_TypeDef; //0x02c 


//================================
#define I2C_ADM_MASK_LSB                    1
#define I2C_ADM_MASK_WIDTH                  7
//================================

typedef union //0x030 
{
    struct
    {
        uint32_t GCEBAB:                             1; // bit0
        uint32_t SADR:                               7; // bit[7:1] --- slaa2 
        uint32_t RSV_END:                           24; // bit[31:8]
    };
    uint32_t Word;
} I2C_SADR2_TypeDef; //0x030 


//================================
#define I2C_GCEBAB_POS                      0
#define I2C_SADR_LSB                        1
#define I2C_SADR_WIDTH                      7
//================================

typedef union //0x034 
{
    struct
    {
        uint32_t RSV_NOUSE1:                         1; // bit0 --- Ignore me
        uint32_t MASK:                               7; // bit[7:1] --- adm2
        uint32_t RSV_END:                           24; // bit[31:8]
    };
    uint32_t Word;
} I2C_ADM2_TypeDef; //0x034 

typedef union //0x038 
{
    struct
    {
        uint32_t GCEBAB:                             1; // bit0
        uint32_t SADR:                               7; // bit[7:1] --- slaa3 
        uint32_t RSV_END:                           24; // bit[31:8]
    };
    uint32_t Word;
} I2C_SADR3_TypeDef; //0x038 


//================================
#define I2C_GCEBAB_POS                      0
#define I2C_SADR_LSB                        1
#define I2C_SADR_WIDTH                      7
//================================

typedef union //0x03c 
{
    struct
    {
        uint32_t RSV_NOUSE1:                         1; // bit0 --- Ignore me
        uint32_t MASK:                               7; // bit[7:1] --- adm3
        uint32_t RSV_END:                           24; // bit[31:8]
    };
    uint32_t Word;
} I2C_ADM3_TypeDef; //0x03c 


//================================
//BLOCK I2C top struct define 
typedef struct
{
    __IO  I2C_CONSET_TypeDef                     CONSET              ; // 0x000, 
                                                                       // i2c control set register  -- i2c control reg 
    __O   I2C_CONCLR_TypeDef                     CONCLR              ; // 0x004, 
                                                                       // i2c control clr register   
    __I   I2C_SR_TypeDef                         SR                  ; // 0x008, 
                                                                       // i2c status register   -- i2c status reg 
    __IO  I2C_DATA_TypeDef                       DATA                ; // 0x00c, 
                                                                       // i2c data register     -- i2c data reg (addr+r/w and data) 8bits 
    __IO  I2C_CLK_TypeDef                        CLK                 ; // 0x010, 
                                                                       // i2c ccrfs register    -- i2c clock (scl) speed contorl reg 
    __IO  I2C_SADR0_TypeDef                      SADR0               ; // 0x014, 
                                                                       // i2c slaa  register    -- slaver address reg 
    __IO  I2C_ADM0_TypeDef                       ADM0                ; // 0x018, 
                                                                       // i2c adm0  register 
    __IO  I2C_XSADR_TypeDef                      XSADR               ; // 0x01c, 
                                                                       // i2c xsla  register    -- extension slaver address reg 
    __IO  I2C_XADM_TypeDef                       XADM                ; // 0x020, 
                                                                       // i2c xadm0 register 
    __O   uint32_t                               SRST                ; // 0x024, 
                                                                       // reserved 
    __IO  I2C_SADR1_TypeDef                      SADR1               ; // 0x028, 
                                                                       // i2c slaa1 register 
    __IO  I2C_ADM1_TypeDef                       ADM1                ; // 0x02c, 
                                                                       // i2c adm1  register  
    __IO  I2C_SADR2_TypeDef                      SADR2               ; // 0x030, 
                                                                       // i2c slaa2 register 
    __IO  I2C_ADM2_TypeDef                       ADM2                ; // 0x034, 
                                                                       // i2c adm2 register 
    __IO  I2C_SADR3_TypeDef                      SADR3               ; // 0x038, 
                                                                       // i2c slaa3 register 
    __IO  I2C_ADM3_TypeDef                       ADM3                ; // 0x03c, 
                                                                       // i2c adm3 register
} I2C_TypeDef;


#define I2C  (( I2C_TypeDef  *)     I2C_BASE)

#endif
