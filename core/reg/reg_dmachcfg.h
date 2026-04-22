#ifndef _REG_DMACHCFG_H_
#define _REG_DMACHCFG_H_

#include "reg_base.h" 

//================================
//BLOCK DMACHCFG define 

#define DMACHCFG_BASE                          ((uint32_t)0x40027000) 
#define DMACHCFG_IFLAG0_ADDR_OFFSET            0x000 
#define DMACHCFG_IFLAG1_ADDR_OFFSET            0x004 
#define DMACHCFG_ISFR0_ADDR_OFFSET             0x008 
#define DMACHCFG_ISFR1_ADDR_OFFSET             0x00c 
#define DMACHCFG_ICFR0_ADDR_OFFSET             0x010 
#define DMACHCFG_ICFR1_ADDR_OFFSET             0x014 
#define DMACHCFG_IEFR0_ADDR_OFFSET             0x018 
#define DMACHCFG_IEFR1_ADDR_OFFSET             0x01c 
#define DMACHCFG_CHSEL0_CONTAIN_ADDR_OFFSET    0x100 
#define DMACHCFG_CHSEL1_CONTAIN_ADDR_OFFSET    0x104 

//================================
//BLOCK DMACHCFG reg struct define 
typedef union //0x100 
{
    struct
    {
        uint32_t CH0_SEL:                            6; // bit[5:0]
        uint32_t RSV_NOUSE1:                         2; // bit[7:6] --- Ignore me
        uint32_t CH1_SEL:                            6; // bit[13:8]
        uint32_t RSV_NOUSE2:                         2; // bit[15:14]-- Ignore me
        uint32_t CH2_SEL:                            6; // bit[21:16]
        uint32_t RSV_NOUSE3:                         2; // bit[23:22]-- Ignore me
        uint32_t CH3_SEL:                            6; // bit[29:24]
        uint32_t RSV_END:                            2; // bit[31:30]
    };
    uint32_t Word;
} DMACHCFG_CHSEL0_CONTAIN_TypeDef; //0x100 


//================================
#define DMACHCFG_CH0_SEL_LSB                0
#define DMACHCFG_CH0_SEL_WIDTH              6
#define DMACHCFG_CH1_SEL_LSB                8
#define DMACHCFG_CH1_SEL_WIDTH              6
#define DMACHCFG_CH2_SEL_LSB                16
#define DMACHCFG_CH2_SEL_WIDTH              6
#define DMACHCFG_CH3_SEL_LSB                24
#define DMACHCFG_CH3_SEL_WIDTH              6
//================================

typedef union //0x104 
{
    struct
    {
        uint32_t CH4_SEL:                            6; // bit[5:0]
        uint32_t RSV_NOUSE1:                         2; // bit[7:6] --- Ignore me
        uint32_t CH5_SEL:                            6; // bit[13:8]
        uint32_t RSV_NOUSE2:                         2; // bit[15:14]-- Ignore me
        uint32_t CH6_SEL:                            6; // bit[21:16]
        uint32_t RSV_NOUSE3:                         2; // bit[23:22]-- Ignore me
        uint32_t CH7_SEL:                            6; // bit[29:24]
        uint32_t RSV_END:                            2; // bit[31:30]
    };
    uint32_t Word;
} DMACHCFG_CHSEL1_CONTAIN_TypeDef; //0x104 


//================================
#define DMACHCFG_CH4_SEL_LSB                0
#define DMACHCFG_CH4_SEL_WIDTH              6
#define DMACHCFG_CH5_SEL_LSB                8
#define DMACHCFG_CH5_SEL_WIDTH              6
#define DMACHCFG_CH6_SEL_LSB                16
#define DMACHCFG_CH6_SEL_WIDTH              6
#define DMACHCFG_CH7_SEL_LSB                24
#define DMACHCFG_CH7_SEL_WIDTH              6
//================================

//================================
//BLOCK DMACHCFG top struct define 
typedef struct
{
    __I   uint32_t                               IFLAG0              ; // 0x000, dma interupt flag 
    __I   uint32_t                               IFLAG1              ; // 0x004, dma err flag 
    __IO  uint32_t                               ISFR0               ; // 0x008, 
    __IO  uint32_t                               ISFR1               ; // 0x00C, 
    __O   uint32_t                               ICFR0               ; // 0x010, clear dma channel interupt 
    __O   uint32_t                               ICFR1               ; // 0x014, clear dma interupt error flag 
    __IO  uint32_t                               IEFR0               ; // 0x018, enable dma channel[x] interupt 
    __IO  uint32_t                               IEFR1               ; // 0x01C, enable dma error interrupt 
    __I   uint32_t                               RSV0[56]            ; // 0x020 -- 0x0FC, Reserved 

    union 
    {
        __IO uint32_t                            CHSEL[2];
      struct
      {
        __IO  DMACHCFG_CHSEL0_CONTAIN_TypeDef    CHSEL0_CONTAIN      ; // 0x100, include channel(0~3) 
        __IO  DMACHCFG_CHSEL1_CONTAIN_TypeDef    CHSEL1_CONTAIN      ; // 0x104, include channel(4~7) 
      };
    };
} DMACHCFG_TypeDef;


#define DMACHCFG  (( DMACHCFG_TypeDef  *)     DMACHCFG_BASE)

#endif
