#ifndef _REG_CACHE_H_
#define _REG_CACHE_H_

#include "reg_base.h" 

//================================
//BLOCK CACHE define 

#define CACHE_BASE                             ((uint32_t)0x19000000) 
#define CACHE_CCR_ADDR_OFFSET                  0x000 
#define CACHE_CCFR_ADDR_OFFSET                 0x004 
#define CACHE_CIR_ADDR_OFFSET                  0x008 
#define CACHE_CRCR0_ADDR_OFFSET                0x00c 
#define CACHE_CRCR1_ADDR_OFFSET                0x010 
#define CACHE_CRCR2_ADDR_OFFSET                0x014 
#define CACHE_CRCR3_ADDR_OFFSET                0x018 

//================================
//BLOCK CACHE reg struct define 
typedef union //0x000 
{
    struct
    {
        uint32_t CACHE_EN:                           1; // bit0 --- 
                                                        // 0: cache disable
                                                        // 1: cache enable
        uint32_t RESERVED:                          31; // bit[31:1]
    };
    uint32_t Word;
} CACHE_CCR_TypeDef; //0x000 


//================================
#define CACHE_CACHE_EN_POS                  0
#define CACHE_RESERVED_LSB                  1
#define CACHE_RESERVED_WIDTH                31
//================================

typedef union //0x004 
{
    struct
    {
        uint32_t RSV_NOUSE1:                         3; // bit[2:0] --- Ignore me
        uint32_t CACHE_INST_DATA:                    1; // bit3 the control bits of cached data type configuration
                                                        // 0: only instruction data can be cached
                                                        // 1: both instruction and common data can be cached
                                                        // note: it requires that arb_xx_hprot[0] indicate whether this access is instruction data
                                                        // request or common data request
        uint32_t RSV_END:                           28; // bit[31:4]
    };
    uint32_t Word;
} CACHE_CCFR_TypeDef; //0x004 


//================================
#define CACHE_CACHE_INST_DATA_POS           3
//================================

typedef union //0x008 
{
    struct
    {
        uint32_t INV_ALL:                            1; // bit0 --- The control bit of invalid all cache lines operation. Set this
                                                        // bit to start an invalid all cache lines operation, and the bit
                                                        // will be cleared when the operation is done.
        uint32_t INV_ONE:                            1; // bit1 --- The control bit of invalid one cache line operation. Set this
                                                        // bit to start an invalid one cache line operation, and the bit
                                                        // will be cleared when the operation is done.
        uint32_t RSV_NOUSE1:                         2; // bit[3:2] --- Ignore me
        uint32_t INV_ADDR:                          28; // bit[31:4]--- The high bits of cache line address of invalid one cache
                                                        // line operation.
    };
    uint32_t Word;
} CACHE_CIR_TypeDef; //0x008 


//================================
#define CACHE_INV_ALL_POS                   0
#define CACHE_INV_ONE_POS                   1
#define CACHE_INV_ADDR_LSB                  4
#define CACHE_INV_ADDR_WIDTH                28
//================================

typedef union //0x00c 
{
    struct
    {
        uint32_t CRCR0_EN:                           1; // bit0 --- Cache region enable bit. Only enable bit is set that the
                                                        // cacheable region is valid.
        uint32_t SIZE:                               5; // bit[5:1] --- These control bits indicate the size of cacheable region.
                                                        // Detail is shown below table.
        uint32_t RSV_NOUSE1:                         4; // bit[9:6] --- Ignore me
        uint32_t BASE_ADDR:                         22; // bit[31:10]-- High 22 bits of base address of cacheable region.
    };
    uint32_t Word;
} CACHE_CRCR0_TypeDef; //0x00c 


//================================
#define CACHE_CRCR0_EN_POS                  0
#define CACHE_SIZE_LSB                      1
#define CACHE_SIZE_WIDTH                    5
#define CACHE_BASE_ADDR_LSB                 10
#define CACHE_BASE_ADDR_WIDTH               22
//================================

typedef union //0x010 
{
    struct
    {
        uint32_t CRCR1_EN:                           1; // bit0 --- Cache region enable bit. Only enable bit is set that the
                                                        // cacheable region is valid.
        uint32_t SIZE:                               5; // bit[5:1] --- These control bits indicate the size of cacheable region.
                                                        // Detail is shown below table.
        uint32_t RSV_NOUSE1:                         4; // bit[9:6] --- Ignore me
        uint32_t BASE_ADDR:                         22; // bit[31:10]-- High 22 bits of base address of cacheable region.
    };
    uint32_t Word;
} CACHE_CRCR1_TypeDef; //0x010 


//================================
#define CACHE_CRCR1_EN_POS                  0
#define CACHE_SIZE_LSB                      1
#define CACHE_SIZE_WIDTH                    5
#define CACHE_BASE_ADDR_LSB                 10
#define CACHE_BASE_ADDR_WIDTH               22
//================================

typedef union //0x014 
{
    struct
    {
        uint32_t CRCR2_EN:                           1; // bit0 --- Cache region enable bit. Only enable bit is set that the
                                                        // cacheable region is valid.
        uint32_t SIZE:                               5; // bit[5:1] --- These control bits indicate the size of cacheable region.
                                                        // Detail is shown below table.
        uint32_t RSV_NOUSE1:                         4; // bit[9:6] --- Ignore me
        uint32_t BASE_ADDR:                         22; // bit[31:10]-- High 22 bits of base address of cacheable region.
    };
    uint32_t Word;
} CACHE_CRCR2_TypeDef; //0x014 


//================================
#define CACHE_CRCR2_EN_POS                  0
#define CACHE_SIZE_LSB                      1
#define CACHE_SIZE_WIDTH                    5
#define CACHE_BASE_ADDR_LSB                 10
#define CACHE_BASE_ADDR_WIDTH               22
//================================

typedef union //0x018 
{
    struct
    {
        uint32_t CRCR3_EN:                           1; // bit0 --- Cache region enable bit. Only enable bit is set that the
                                                        // cacheable region is valid.
        uint32_t SIZE:                               5; // bit[5:1] --- These control bits indicate the size of cacheable region.
                                                        // Detail is shown below table.
        uint32_t RSV_NOUSE1:                         4; // bit[9:6] --- Ignore me
        uint32_t BASE_ADDR:                         22; // bit[31:10]-- High 22 bits of base address of cacheable region.
    };
    uint32_t Word;
} CACHE_CRCR3_TypeDef; //0x018 


//================================
#define CACHE_CRCR3_EN_POS                  0
#define CACHE_SIZE_LSB                      1
#define CACHE_SIZE_WIDTH                    5
#define CACHE_BASE_ADDR_LSB                 10
#define CACHE_BASE_ADDR_WIDTH               22
//================================

//================================
//BLOCK CACHE top struct define 
typedef struct
{
    __IO  CACHE_CCR_TypeDef                      CCR                 ; // 0x000, 
                                                                       // This register is responsible for making cache enable or disable. 
    __IO  CACHE_CCFR_TypeDef                     CCFR                ; // 0x004, 
                                                                       // This register controls the software configuration of cache. Note: The cache should be disabled before
                                                                       // changing the configuration, or fatal errors may occur. 
    __IO  CACHE_CIR_TypeDef                      CIR                 ; // 0x008, 
                                                                       // This register controls the invalid operation for cache, including invalid one cache line and invalid all. 
    __IO  CACHE_CRCR0_TypeDef                    CRCR0               ; // 0x00c, 
                                                                       // This register controls the cacheable region. Request address must locate in the cacheable region and this
                                                                       // region is enabled, or read data will not be cached. 
    __IO  CACHE_CRCR1_TypeDef                    CRCR1               ; // 0x010,  
    __IO  CACHE_CRCR2_TypeDef                    CRCR2               ; // 0x014,  
    __IO  CACHE_CRCR3_TypeDef                    CRCR3               ; // 0x018,  
} CACHE_TypeDef;


#define CACHE  (( CACHE_TypeDef  *)     CACHE_BASE)

#endif
