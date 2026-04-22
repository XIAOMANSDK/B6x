#ifndef _REG_GPIO_H_
#define _REG_GPIO_H_

#include "reg_base.h" 

//================================
//BLOCK GPIO define 

#define GPIO_BASE                              ((uint32_t)0x40010000) 
#define GPIO_DAT_SET_ADDR_OFFSET               0x000 
#define GPIO_DAT_CLR_ADDR_OFFSET               0x004 
#define GPIO_DAT_TOG_ADDR_OFFSET               0x008 
#define GPIO_DAT_MSK_ADDR_OFFSET               0x00c 
#define GPIO_DAT_ADDR_OFFSET                   0x010 
#define GPIO_PIN_ADDR_OFFSET                   0x014 
#define GPIO_DIR_SET_ADDR_OFFSET               0x020 
#define GPIO_DIR_CLR_ADDR_OFFSET               0x024 
#define GPIO_DIR_TOG_ADDR_OFFSET               0x028 
#define GPIO_DIR_MSK_ADDR_OFFSET               0x02c 
#define GPIO_DIR_ADDR_OFFSET                   0x030 

//================================
//BLOCK GPIO top struct define 
typedef struct
{
    __O   uint32_t                               DAT_SET; // 0x000,  
                                                          // WR: (1: gpio is set high level, 0: no effect)
    __O   uint32_t                               DAT_CLR; // 0x004, 
                                                          // WR: (1: gpio is set low level, 0: no effect)
    __O   uint32_t                               DAT_TOG; // 0x008, 
                                                          // WR: (1: gpio state is overturn (1->0 or 0->1)) 
    __IO  uint32_t                               DAT_MSK; // 0x00c,  
                                                          // WR: (1: dat_set, dat_clr, dat_tog, dat can not set, 0: no effect)
                                                          // RD: () 
    __IO  uint32_t                               DAT    ; // 0x010, 
                                                          // WR: (1: write this register equal to write dat_set, dat_clr, dat_tog)
                                                          // RD: (return current register value)
    __I   uint32_t                               PIN    ; // 0x014,  
                                                          // RD: (get gpio current state(1: high level, 0: low level))
    __I   uint32_t                               RSV0[2];
    __O   uint32_t                               DIR_SET; // 0x020,  
                                                          // WR: (1: set gpio is output, 0: no effect) 
    __O   uint32_t                               DIR_CLR; // 0x024, 
                                                          // WR: (1: clear gpio output function, 0: no effect) 
    __O   uint32_t                               DIR_TOG; // 0x028, 
                                                          // WR: (1: gpio direction is overturn (1->0 or 0->1)) 
    __IO  uint32_t                               DIR_MSK; // 0x02c, 
                                                          // WR: (1: dir_set, dir_clr, dir_tog, dir can not set, 0: no effect)
                                                          // RD: ()                                                            
    __IO  uint32_t                               DIR    ; // 0x030,  
                                                          // WR: (write this register equal to write dir_set, dir_clr, dir_tog)
                                                          // RD: (get gpio current direction (1: output, 0: input))
} GPIO_TypeDef;


#define GPIO  (( GPIO_TypeDef  *)     GPIO_BASE)

#endif
