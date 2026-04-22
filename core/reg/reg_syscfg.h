#ifndef _REG_SYSCFG_H_
#define _REG_SYSCFG_H_

#include "reg_base.h" 

//================================
//BLOCK SYSCFG define 

#define SYSCFG_BASE                             ((uint32_t)0x40001000) 
#define SYSCFG_BB_PMU_ENABLE_ADDR_OFFSET        0x000 
#define SYSCFG_AHB_MATRIX_PRIORITY_ADDR_OFFSET  0x004 
#define SYSCFG_DBG_CTRL_ADDR_OFFSET             0x008 
#define SYSCFG_USB_CTRL_ADDR_OFFSET             0x00c 
#define SYSCFG_ROM_CTRL_ADDR_OFFSET             0x010 
#define SYSCFG_SRAM_CTRL_ADDR_OFFSET            0x014 
#define SYSCFG_ROM_SIGNATURE_TARGET_ADDR_OFFSET 0x018 
#define SYSCFG_BIST_MODE_ADDR_OFFSET            0x01c 
#define SYSCFG_BIST_STATUS_ADDR_OFFSET          0x020 
#define SYSCFG_BIST_STATUS_CLR_ADDR_OFFSET      0x024 
#define SYSCFG_ROM_SIGNATURE_RPT_ADDR_OFFSET    0x028 
#define SYSCFG_CACHSRAM_CFG_ADDR_OFFSET         0x02c 
#define SYSCFG_EM_BASE_CFG_ADDR_OFFSET          0x030 
#define SYSCFG_CACHE_ACCESS_CNT_ADDR_OFFSET     0x034 
#define SYSCFG_CACHE_HIT_CNT_ADDR_OFFSET        0x038 
#define SYSCFG_ACC_CCR_BUSY_ADDR_OFFSET         0x03c 
#define SYSCFG_MFV_ARRAY_D_ADDR_OFFSET          0x040 
#define SYSCFG_SYS_BACKUP0_ADDR_OFFSET          0x044 

//================================
//BLOCK SYSCFG reg struct define 
typedef union //0x008 
{
    struct
    {
        uint32_t SYS_DEBUG_SEL:                      2; // bit[1:0] ---
                                                        // 2'b00: rf_debug port enable
                                                        // 2'b01: BB debug port enable
                                                        // 2'b10: MODEM debug port enable
                                                        // 2'b11: USB debug port enable
        uint32_t RF_EXT_EN:                          1; // bit2 --- chip set as RF IP block
                                                        // 0: rf reg is controlled by on chip logic;
                                                        // 1: rf reg is controlled by external through SPI interface
        uint32_t RF_SPI_EN:                          1; // bit3 ---
                                                        // 0: RF reg is direct controlled by APB interface
                                                        // 1: RF reg is controlled by SPI interface
                                                        // note
                                                        // {rf_ext_en,rf_spi_en}
                                                        // 00: rf reg is controlled by on chip APB interface
                                                        // 01: rf reg is controlled by BB througth SPI interface
                                                        // 11: rf reg is controlled by external Chip through SPI interface
                                                        // 10: forbidden
        uint32_t IWDT_DEBUG:                         1; // bit4
        uint32_t USB_DBG_SEL:                        2; // bit[6:5]
        uint32_t RSV_END:                           25; // bit[31:7]
    };
    uint32_t Word;
} SYSCFG_DBG_CTRL_TypeDef; //0x008 


//================================
#define SYSCFG_SYS_DEBUG_SEL_LSB            0
#define SYSCFG_SYS_DEBUG_SEL_WIDTH          2
#define SYSCFG_RF_EXT_EN_POS                2
#define SYSCFG_RF_SPI_EN_POS                3
#define SYSCFG_IWDT_DEBUG_POS               4
#define SYSCFG_USB_DBG_SEL_LSB              5
#define SYSCFG_USB_DBG_SEL_WIDTH            2
//================================

typedef union //0x00c 
{
    struct
    {
        uint32_t USB_SOF_INT_EN:                     1; // bit0 --- when is 1, USB SOF signal can be used as interrupt
        uint32_t USB_PHY_MOD:                        1; // bit1 ---
                                                        // 1: the chip is set as USB PHY
        uint32_t DIG_USB_PU:                         2; // bit[3:2] --- set as 1 when USB function enable
                                                        // when is 1, enable pullup 1.5K for DP  
        uint32_t DIG_USB_RXEN:                       1; // bit4
        uint32_t RSV_NOUSE1:                         3; // bit[7:5] --- Ignore me
        uint32_t USB_NRSTO:                          1; // bit8     --- when is 1, USB is exit reset
        uint32_t USB_SUSPEND:                        1; // bit9
        uint32_t RSV_END:                           22; // bit[31:10]
    };
    uint32_t Word;
} SYSCFG_USB_CTRL_TypeDef; //0x00c 


//================================
#define SYSCFG_USB_SOF_INT_EN_POS           0
#define SYSCFG_USB_PHY_MOD_POS              1
#define SYSCFG_DIG_USB_PU_LSB               2
#define SYSCFG_DIG_USB_PU_WIDTH             2
#define SYSCFG_DIG_USB_RXEN_POS             4
#define SYSCFG_USB_NRSTO_POS                8
#define SYSCFG_USB_SUSPEND_POS              9
//================================

typedef union //0x010 
{
    struct
    {
        uint32_t ROM_RDMG:                           4; // bit[3:0]
        uint32_t ROM_RDMG_EN:                        1; // bit4
        uint32_t MEM_SCAN_IN:                        1; // bit5
        uint32_t RSV_NOUSE1:                         2; // bit[7:6] --- Ignore me
        uint32_t MDM_RDMG:                           3; // bit[10:8]
        uint32_t MDM_RDMG_EN:                        1; // bit11
        uint32_t RSV_END:                           20; // bit[31:12]
    };
    uint32_t Word;
} SYSCFG_ROM_CTRL_TypeDef; //0x010 


//================================
#define SYSCFG_ROM_RDMG_LSB                 0
#define SYSCFG_ROM_RDMG_WIDTH               4
#define SYSCFG_ROM_RDMG_EN_POS              4
#define SYSCFG_MEM_SCAN_IN_POS              5
#define SYSCFG_MDM_RDMG_LSB                 8
#define SYSCFG_MDM_RDMG_WIDTH               3
#define SYSCFG_MDM_RDMG_EN_POS              11
//================================

typedef union //0x014 
{
    struct
    {
        uint32_t TAG_RDMG:                           4; // bit[3:0]
        uint32_t CACHE_RDMG:                         4; // bit[7:4]
        uint32_t EM_RDMG:                            4; // bit[11:8]
        uint32_t SADCMEM_RDMG:                       4; // bit[15:12]
        uint32_t USBMEM_RDMG:                        4; // bit[19:16]
        uint32_t DSRAM_RDMG:                         3; // bit[22:20]
        uint32_t RSV_NOUSE1:                         1; // bit23 --- Ignore me
        uint32_t TAG_RDMG_EN:                        1; // bit24
        uint32_t CACHE_RDMG_EN:                      1; // bit25
        uint32_t EM_RDMG_EN:                         1; // bit26
        uint32_t SADCMEM_RDMG_EN:                    1; // bit27
        uint32_t USBMEM_RDMG_EN:                     1; // bit28
        uint32_t DSRAM_RDMG_EN:                      1; // bit29
        uint32_t RSV_END:                            2; // bit[31:30]
    };
    uint32_t Word;
} SYSCFG_SRAM_CTRL_TypeDef; //0x014 


//================================
#define SYSCFG_TAG_RDMG_LSB                 0
#define SYSCFG_TAG_RDMG_WIDTH               4
#define SYSCFG_CACHE_RDMG_LSB               4
#define SYSCFG_CACHE_RDMG_WIDTH             4
#define SYSCFG_EM_RDMG_LSB                  8
#define SYSCFG_EM_RDMG_WIDTH                4
#define SYSCFG_SADCMEM_RDMG_LSB             12
#define SYSCFG_SADCMEM_RDMG_WIDTH           4
#define SYSCFG_USBMEM_RDMG_LSB              16
#define SYSCFG_USBMEM_RDMG_WIDTH            4
#define SYSCFG_DSRAM_RDMG_LSB               20
#define SYSCFG_DSRAM_RDMG_WIDTH             3
#define SYSCFG_TAG_RDMG_EN_POS              24
#define SYSCFG_CACHE_RDMG_EN_POS            25
#define SYSCFG_EM_RDMG_EN_POS               26
#define SYSCFG_SADCMEM_RDMG_EN_POS          27
#define SYSCFG_USBMEM_RDMG_EN_POS           28
#define SYSCFG_DSRAM_RDMG_EN_POS            29
//================================

//================================
//BLOCK SYSCFG top struct define 
typedef struct
{
    __IO  uint32_t                               BB_PMU_ENABLE       ; // 0x000,  
    __IO  uint32_t                               AHB_MATRIX_PRIORITY ; // 0x004,  
    __IO  SYSCFG_DBG_CTRL_TypeDef                DBG_CTRL            ; // 0x008,  
    __IO  SYSCFG_USB_CTRL_TypeDef                USB_CTRL            ; // 0x00c,  
    __IO  SYSCFG_ROM_CTRL_TypeDef                ROM_CTRL            ; // 0x010,  
    __IO  SYSCFG_SRAM_CTRL_TypeDef               SRAM_CTRL           ; // 0x014,  
    __IO  uint32_t                               ROM_SIGNATURE_TARGET; // 0x018, 
                                                                       // set rom_target_signature value before bist_mode as 1 
    __IO  uint32_t                               BIST_MODE           ; // 0x01c, 
                                                                       // all sram and rom bist start
                                                                       // note: bist follow
                                                                       // mode 1 : get report from GPIO
                                                                       //     1) : config GPIO for glb_bist_done and glb_bist_fail
                                                                       //     2) : set rom_signature_target value
                                                                       //     3) : set bist_mode as 1
                                                                       //     4) : wait a fixed time
                                                                       //     5) : check the glb_bist_done and glb_bist_fail state through GPIO
                                                                       //
                                                                       // mode 2 : get report from bist_statues 
                                                                       //     1) : set rom_signature_target value
                                                                       //     2) : set bist_mode as 1
                                                                       //     3) : wait a fixed time
                                                                       //     4) : reset chip, and reconnect chip through UART 
                                                                       //     5) : check the glb_bist_done and glb_bist_fail state through bist_status[1:0]
                                                                       // option step 6) : set bist_status_clr as 1 for clear status 
    __I   uint32_t                               BIST_STATUS         ; // 0x020, 
                                                                       // [0] global bist done
                                                                       // [1] global bist fail
                                                                       // [2] em_bist_done
                                                                       // [3] sram_bist_done
                                                                       // [4] sadcmem_bist_done 
                                                                       // [5] mdm_bist_done 
                                                                       // [6] rom_bist_done 
                                                                       // [7] em_bist_fail
                                                                       // [8] sram_bist_fail
                                                                       // [9] sadcmem_bist_fail
                                                                       // [10] mdmmem_bist_fail
                                                                       // [11] rom_bist_fail 
    __O   uint32_t                               BIST_STATUS_CLR     ; // 0x024,  
    __I   uint32_t                               ROM_SIGNATURE_RPT   ; // 0x028, 
                                                                       // rom signature report, just use for debug 
    __IO  uint32_t                               CACHSRAM_CFG        ; // 0x02c, 
                                                                       // 0: the 4KB sram use as CACHE
                                                                       // 1: the 4KB sram use as DATA SRAM: 0x2000_6000 ~ 0x2000_6FFF 
    __IO  uint32_t                               EM_BASE_CFG         ; // 0x030, 
                                                                       // 1KB step
                                                                       // 0: BB EM base addr: 0x2000_8000, EM size : 8KB
                                                                       // 1: BB EM base addr: 0x2000_8400, EM size : 7KB
                                                                       // 2: BB EM base addr: 0x2000_8800, EM size : 6KB
                                                                       // 3: BB EM base addr: 0x2000_8c00, EM size : 5KB
                                                                       // 4: BB EM base addr: 0x2000_9000, EM size : 4KB
                                                                       // 5: BB EM base addr: 0x2000_9400, EM size : 3KB
                                                                       // 6: BB EM base addr: 0x2000_9800, EM size : 2KB
                                                                       // 7: BB EM base addr: 0x2000_9c00, EM size : 1KB 
    __I   uint32_t                               CACHE_ACCESS_CNT    ; // 0x034,  
    __I   uint32_t                               CACHE_HIT_CNT       ; // 0x038,  
    __I   uint32_t                               ACC_CCR_BUSY        ; // 0x03c, 
                                                                       // cache status  
    __I   uint32_t                               MFV_ARRAY_D         ; // 0x040,  
    __IO  uint32_t                               SYS_BACKUP0         ; // 0x044, 
                                                                       // only reset by core por12_core_stb 
} SYSCFG_TypeDef;


#define SYSCFG  (( SYSCFG_TypeDef  *)     SYSCFG_BASE)

#endif
