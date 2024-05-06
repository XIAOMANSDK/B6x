#ifndef _REG_IWDT_H_
#define _REG_IWDT_H_

#include "reg_base.h" 

//================================
//BLOCK IWDT define 

#define IWDT_BASE                              ((uint32_t)0x4002a000) 
#define IWDT_LOAD_ADDR_OFFSET                  0x000 
#define IWDT_VALUE_ADDR_OFFSET                 0x004 
#define IWDT_CTRL_ADDR_OFFSET                  0x008 
#define IWDT_INTCLR_ADDR_OFFSET                0x00c 
#define IWDT_RIS_ADDR_OFFSET                   0x010 
#define IWDT_LOCK_ADDR_OFFSET                  0x100 

//================================
//BLOCK IWDT reg struct define 
typedef union //0x008 
{
    struct
    {
        uint32_t EN:                                 1; // bit0 --- iwdg enable
        uint32_t INTEN:                              1; // bit1 --- iwdg interrupt enable
        uint32_t RSTEN:                              1; // bit2 --- iwdg reset enable
        uint32_t CLKSEL:                             1; // bit3 --- iwdg clk select
                                                        // 0:iwdt clk select apbclk
                                                        // 1:iwdt clk select lsi
        uint32_t RSV_END:                           28; // bit[31:4]
    };
    uint32_t Word;
} IWDT_CTRL_TypeDef; //0x008 


//================================
#define IWDT_EN_POS                         0
#define IWDT_INTEN_POS                      1
#define IWDT_RSTEN_POS                      2
#define IWDT_CLKSEL_POS                     3
//================================

//================================
//BLOCK IWDT top struct define 
typedef struct
{
    __IO  uint32_t                               LOAD    ; // 0x000, 
                                                           // reload  value register 
    __I   uint32_t                               VALUE   ; // 0x004, 
                                                           // current value register 
    __IO  IWDT_CTRL_TypeDef                      CTRL    ; // 0x008, 
                                                           // control register 
    __O   uint32_t                               INTCLR  ; // 0x00c, 
                                                           // interrupt flag clear register 
    __I   uint32_t                               RIS     ; // 0x010, 
                                                           // interrupt flag register 
    __I   uint32_t                               RSV0[59];
    __IO  uint32_t                               LOCK    ; // 0x100, 
                                                           // iwdg register locking register
                                                           // write 0x1ACCE551 to unlock  
} IWDT_TypeDef;


#define IWDT  (( IWDT_TypeDef  *)     IWDT_BASE)

#endif
