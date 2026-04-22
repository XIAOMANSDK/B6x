#ifndef _REG_CSC_H_
#define _REG_CSC_H_

#include "reg_base.h" 

//================================
//BLOCK CSC define 


#define CSC_BASE                               ((uint32_t)0x40002000) 
#define CSC_OUT_CTRL0_ADDR_OFFSET              0x000 
#define CSC_OUT_CTRL1_ADDR_OFFSET              0x004 
#define CSC_OUT_CTRL2_ADDR_OFFSET              0x008 
#define CSC_OUT_CTRL3_ADDR_OFFSET              0x00c 
#define CSC_OUT_CTRL4_ADDR_OFFSET              0x010 
#define CSC_OUT_CTRL5_ADDR_OFFSET              0x014 
#define CSC_OUT_CTRL6_ADDR_OFFSET              0x018 
#define CSC_OUT_CTRL7_ADDR_OFFSET              0x01c 
#define CSC_OUT_CTRL8_ADDR_OFFSET              0x020 
#define CSC_OUT_CTRL9_ADDR_OFFSET              0x024 
#define CSC_OUT_CTRL10_ADDR_OFFSET             0x028 
#define CSC_OUT_CTRL11_ADDR_OFFSET             0x02c 
#define CSC_OUT_CTRL12_ADDR_OFFSET             0x030 
#define CSC_OUT_CTRL13_ADDR_OFFSET             0x034 
#define CSC_OUT_CTRL14_ADDR_OFFSET             0x038 
#define CSC_OUT_CTRL15_ADDR_OFFSET             0x03c 
#define CSC_OUT_CTRL16_ADDR_OFFSET             0x040 
#define CSC_OUT_CTRL17_ADDR_OFFSET             0x044 
#define CSC_OUT_CTRL18_ADDR_OFFSET             0x048 
#define CSC_OUT_CTRL19_ADDR_OFFSET             0x04c 
#define CSC_IN_CTRL0_ADDR_OFFSET               0x080 
#define CSC_IN_CTRL1_ADDR_OFFSET               0x084 
#define CSC_IN_CTRL2_ADDR_OFFSET               0x088 
#define CSC_IN_CTRL3_ADDR_OFFSET               0x08c 
#define CSC_IN_CTRL4_ADDR_OFFSET               0x090 
#define CSC_IN_CTRL5_ADDR_OFFSET               0x094 
#define CSC_IN_CTRL6_ADDR_OFFSET               0x098 
#define CSC_IN_CTRL7_ADDR_OFFSET               0x09c 
#define CSC_IN_CTRL8_ADDR_OFFSET               0x0a0 
#define CSC_IN_CTRL9_ADDR_OFFSET               0x0a4 
#define CSC_IN_CTRL10_ADDR_OFFSET              0x0a8 
#define CSC_IN_CTRL11_ADDR_OFFSET              0x0ac 
#define CSC_IN_CTRL12_ADDR_OFFSET              0x0b0 
#define CSC_IN_CTRL13_ADDR_OFFSET              0x0b4 
#define CSC_PIOA00_CTRL_ADDR_OFFSET            0x100 
#define CSC_PIOA01_CTRL_ADDR_OFFSET            0x104 
#define CSC_PIOA02_CTRL_ADDR_OFFSET            0x108 
#define CSC_PIOA03_CTRL_ADDR_OFFSET            0x10c 
#define CSC_PIOA04_CTRL_ADDR_OFFSET            0x110 
#define CSC_PIOA05_CTRL_ADDR_OFFSET            0x114 
#define CSC_PIOA06_CTRL_ADDR_OFFSET            0x118 
#define CSC_PIOA07_CTRL_ADDR_OFFSET            0x11c 
#define CSC_PIOA08_CTRL_ADDR_OFFSET            0x120 
#define CSC_PIOA09_CTRL_ADDR_OFFSET            0x124 
#define CSC_PIOA10_CTRL_ADDR_OFFSET            0x128 
#define CSC_PIOA11_CTRL_ADDR_OFFSET            0x12c 
#define CSC_PIOA12_CTRL_ADDR_OFFSET            0x130 
#define CSC_PIOA13_CTRL_ADDR_OFFSET            0x134 
#define CSC_PIOA14_CTRL_ADDR_OFFSET            0x138 
#define CSC_PIOA15_CTRL_ADDR_OFFSET            0x13c 
#define CSC_PIOA16_CTRL_ADDR_OFFSET            0x140 
#define CSC_PIOA17_CTRL_ADDR_OFFSET            0x144 
#define CSC_PIOA18_CTRL_ADDR_OFFSET            0x148 
#define CSC_PIOA19_CTRL_ADDR_OFFSET            0x14c 
#define CSC_FSH_IO_CTRL_RUN_ADDR_OFFSET        0x180 
#define CSC_FSH_IO_CTRL_DP_ADDR_OFFSET         0x184 


typedef union  
{
    struct
    {
        uint32_t FSEL:                           3; // bit[2:0] --- function select
        uint32_t RSV_NOUSE1:                     2; // bit[4:3] --- Ignore me
        uint32_t DS:                             1; // bit5     --- drive strength mode
        uint32_t PUPDCTRL:                       2; // bit[7:6] --- pull up/down
        uint32_t IE:                             1; // bit8     --- input enable
        uint32_t CE:                             1; // bit9     --- 1ma current spurce enable
        uint32_t AE:                             1; // bit10    --- analog enable
        uint32_t ODE:                            1; // bit11    --- open drain enable
        uint32_t RSV_END:                       20; // bit[31:12]
    };
    uint32_t Word;
} CSC_IO_CTRL_TypeDef; 

//================================
#define CSC_IO_FSEL_LSB                 0
#define CSC_IO_FSEL_WIDTH               3
#define CSC_IO_DS_POS                   5
#define CSC_IO_PUPDCTRL_LSB             6
#define CSC_IO_PUPDCTRL_WIDTH           2
#define CSC_IO_IE_POS                   8
#define CSC_IO_CE_POS                   9
#define CSC_IO_AE_POS                   10
#define CSC_IO_ODE_POS                  11
//================================
//
typedef union //0x000 
{
    struct
    {
        uint32_t CSC_FSEL:                       5; // bit[4:0] --- pad output function select
                                                    // padio0 sel pad port, for example:pioa00,pioa01...
        uint32_t RSV_NOUSE1:                     2; // bit[6:5] --- Ignore me
        uint32_t CSC_FEN:                        1; // bit7     --- csc peripheral function enable
        uint32_t RSV_END:                       24; // bit[31:8]
    };
    uint32_t Word;
} CSC_OUTPUT_CTRL_TypeDef;  

//================================
#define CSC_FOUT_SEL_LSB               0
#define CSC_FOUT_SEL_WIDTH             5
#define CSC_FOUT_EN_POS                7
//================================

typedef union //0x000 
{
    struct
    {
        uint32_t CSC_FSEL:                       5; // bit[4:0] --- pad input function select
                                                    // padio0 sel pad port, for example:pioa00,pioa01...
        uint32_t RSV_NOUSE1:                     2; // bit[6:5] --- Ignore me
        uint32_t CSC_FEN:                        1; // bit7     --- csc peripheral function enable
        uint32_t RSV_END:                       24; // bit[31:8]
    };
    uint32_t Word;
} CSC_INPUT_CTRL_TypeDef;  

//================================
#define CSC_FIN_SEL_LSB               0
#define CSC_FIN_SEL_WIDTH             5
#define CSC_FIN_EN_POS                7
//================================

typedef union //0x180
{
    struct
    {
        uint32_t QSCK_OE_EN_RUN:                   1; // bit0
        uint32_t QSCK_PUPDCTRL_RUN:                2; // bit[2:1]
        uint32_t QSCK_IE_RUN:                      1; // bit3
        uint32_t QCSN_OE_EN_RUN:                   1; // bit4
        uint32_t QCSN_PUPDCTRL_RUN:                2; // bit[6:5]
        uint32_t QCSN_IE_RUN:                      1; // bit7
        uint32_t QIO0_OE_EN_RUN:                   1; // bit8
        uint32_t QIO0_PUPDCTRL_RUN:                2; // bit[10:9]
        uint32_t QIO0_IE_RUN:                      1; // bit11
        uint32_t QIO1_OE_EN_RUN:                   1; // bit12
        uint32_t QIO1_PUPDCTRL_RUN:                2; // bit[14:13]
        uint32_t QIO1_IE_RUN:                      1; // bit15
        uint32_t QIO2_OE_EN_RUN:                   1; // bit16
        uint32_t QIO2_PUPDCTRL_RUN:                2; // bit[18:17]
        uint32_t QIO2_IE_RUN:                      1; // bit19
        uint32_t QIO3_OE_EN_RUN:                   1; // bit20
        uint32_t QIO3_PUPDCTRL_RUN:                2; // bit[22:21]
        uint32_t QIO3_IE_RUN:                      1; // bit23
        uint32_t QSCK_DS_RUN:                      1; // bit24
        uint32_t QCSN_DS_RUN:                      1; // bit25
        uint32_t QIO0_DS_RUN:                      1; // bit26
        uint32_t QIO1_DS_RUN:                      1; // bit27
        uint32_t QIO2_DS_RUN:                      1; // bit28
        uint32_t QIO3_DS_RUN:                      1; // bit29
        uint32_t RSV_END:                          2; // bit[31:30]
    };
    uint32_t Word;
} CSC_FSH_PAD_RUN_CTRL_TypeDef; //0x180 

//================================
#define CSC_QSCK_OE_EN_RUN_POS           0
#define CSC_QSCK_PUPDCTRL_RUN_LSB        1
#define CSC_QSCK_PUPDCTRL_RUN_WIDTH      2
#define CSC_QSCK_IE_RUN_POS              3
#define CSC_QCSN_OE_EN_RUN_POS           4
#define CSC_QCSN_PUPDCTRL_RUN_LSB        5
#define CSC_QCSN_PUPDCTRL_RUN_WIDTH      2
#define CSC_QCSN_IE_RUN_POS              7
#define CSC_QIO0_OE_EN_RUN_POS           8
#define CSC_QIO0_PUPDCTRL_RUN_LSB        9
#define CSC_QIO0_PUPDCTRL_RUN_WIDTH      2
#define CSC_QIO0_IE_RUN_POS              11
#define CSC_QIO1_OE_EN_RUN_POS           12
#define CSC_QIO1_PUPDCTRL_RUN_LSB        13
#define CSC_QIO1_PUPDCTRL_RUN_WIDTH      2
#define CSC_QIO1_IE_RUN_POS              15
#define CSC_QIO2_OE_EN_RUN_POS           16
#define CSC_QIO2_PUPDCTRL_RUN_LSB        17
#define CSC_QIO2_PUPDCTRL_RUN_WIDTH      2
#define CSC_QIO2_IE_RUN_POS              19
#define CSC_QIO3_OE_EN_RUN_POS           20
#define CSC_QIO3_PUPDCTRL_RUN_LSB        21
#define CSC_QIO3_PUPDCTRL_RUN_WIDTH      2
#define CSC_QIO3_IE_RUN_POS              23
#define CSC_QSCK_DS_RUN_POS              24
#define CSC_QCSN_DS_RUN_POS              25
#define CSC_QIO0_DS_RUN_POS              26
#define CSC_QIO1_DS_RUN_POS              27
#define CSC_QIO2_DS_RUN_POS              28
#define CSC_QIO3_DS_RUN_POS              29
//================================

typedef union //0x184 
{
    struct
    {
        uint32_t QSCK_OE_EN_DP:                   1; // bit0
        uint32_t QSCK_PUPDCTRL_DP:                2; // bit[2:1]
        uint32_t QSCK_IE_DP:                      1; // bit3
        uint32_t QCSN_OE_EN_DP:                   1; // bit4
        uint32_t QCSN_PUPDCTRL_DP:                2; // bit[6:5]
        uint32_t QCSN_IE_DP:                      1; // bit7
        uint32_t QIO0_OE_EN_DP:                   1; // bit8
        uint32_t QIO0_PUPDCTRL_DP:                2; // bit[10:9]
        uint32_t QIO0_IE_DP:                      1; // bit11
        uint32_t QIO1_OE_EN_DP:                   1; // bit12
        uint32_t QIO1_PUPDCTRL_DP:                2; // bit[14:13]
        uint32_t QIO1_IE_DP:                      1; // bit15
        uint32_t QIO2_OE_EN_DP:                   1; // bit16
        uint32_t QIO2_PUPDCTRL_DP:                2; // bit[18:17]
        uint32_t QIO2_IE_DP:                      1; // bit19
        uint32_t QIO3_OE_EN_DP:                   1; // bit20
        uint32_t QIO3_PUPDCTRL_DP:                2; // bit[22:21]
        uint32_t QIO3_IE_DP:                      1; // bit23
        uint32_t RSV_END:                         8; // bit[31:24]
    };
    uint32_t Word;
} CSC_FSH_PAD_DP_CTRL_TypeDef; //0x184

//================================
#define CSC_QSCK_OE_EN_DP_POS            0
#define CSC_QSCK_PUPDCTRL_DP_LSB         1
#define CSC_QSCK_PUPDCTRL_DP_WIDTH       2
#define CSC_QSCK_IE_DP_POS               3
#define CSC_QCSN_OE_EN_DP_POS            4
#define CSC_QCSN_PUPDCTRL_DP_LSB         5
#define CSC_QCSN_PUPDCTRL_DP_WIDTH       2
#define CSC_QCSN_IE_DP_POS               7
#define CSC_QIO0_OE_EN_DP_POS            8
#define CSC_QIO0_PUPDCTRL_DP_LSB         9
#define CSC_QIO0_PUPDCTRL_DP_WIDTH       2
#define CSC_QIO0_IE_DP_POS               11
#define CSC_QIO1_OE_EN_DP_POS            12
#define CSC_QIO1_PUPDCTRL_DP_LSB         13
#define CSC_QIO1_PUPDCTRL_DP_WIDTH       2
#define CSC_QIO1_IE_DP_POS               15
#define CSC_QIO2_OE_EN_DP_POS            16
#define CSC_QIO2_PUPDCTRL_DP_LSB         17
#define CSC_QIO2_PUPDCTRL_DP_WIDTH       2
#define CSC_QIO2_IE_DP_POS               19
#define CSC_QIO3_OE_EN_DP_POS            20
#define CSC_QIO3_PUPDCTRL_DP_LSB         21
#define CSC_QIO3_PUPDCTRL_DP_WIDTH       2
#define CSC_QIO3_IE_DP_POS               23
//================================

//================================
//BLOCK CSC top struct define 
typedef struct
{
    __IO  CSC_OUTPUT_CTRL_TypeDef              CSC_OUTPUT[20]      ; // 0x000 ~ 0x04C,
    __I   uint32_t                             RSV0[12]            ; // 0x050 ~ 0x07C,
    __IO  CSC_INPUT_CTRL_TypeDef               CSC_INPUT[14]       ; // 0x080 ~ 0x0B4,
    __I   uint32_t                             RSV1[18]            ; // 0x0B8 ~ 0x0FC,
    __IO  CSC_IO_CTRL_TypeDef                  CSC_PIO[20]         ; // 0x100 ~ 0x14C,
    __I   uint32_t                             RSV2[12]            ; // 0x150 ~ 0x17C
    __IO  CSC_FSH_PAD_RUN_CTRL_TypeDef         FSH_PAD_RUN_CTRL    ; // 0x180,  
    __IO  CSC_FSH_PAD_DP_CTRL_TypeDef          FSH_PAD_DP_CTRL     ; // 0x184,  
} CSC_TypeDef;


#define CSC  (( CSC_TypeDef  *)     CSC_BASE)

#endif
