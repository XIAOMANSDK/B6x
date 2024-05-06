#ifndef _REG_AON_H_
#define _REG_AON_H_

#include "reg_base.h" 

//================================
//BLOCK AON define 

#define AON_BASE                               ((uint32_t)0x40030000)
#define AON_BACKUP0_ADDR_OFFSET                0x000
#define AON_BACKUP1_ADDR_OFFSET                0x004
#define AON_BKHOLD_CTRL_ADDR_OFFSET            0x008
#define AON_BACKUP2_ADDR_OFFSET                0x00c
#define AON_BLE_RTC_CTL_ADDR_OFFSET            0x010
#define AON_SCAN_MODE_CTRL_ADDR_OFFSET         0x014
#define AON_PMU_CTRL_ADDR_OFFSET               0x018
#define AON_PMU_WKUP_CTRL_ADDR_OFFSET          0x01c
#define AON_PMU_WKUP_ST_ADDR_OFFSET            0x020
#define AON_PMU_ANA_CTRL_ADDR_OFFSET           0x024
#define AON_XOSC16M_CTRL_ADDR_OFFSET           0x028

//================================
//BLOCK AON reg struct define 

typedef union //0x008 
{
    struct
    {
        uint32_t RC32K_MSB_TRIM_CFG:                 4; // bit[3:0]
        uint32_t RC32K_LSB_TRIM_CFG:                10; // bit[13:4]
        uint32_t XOSC_EST_MODE:                      1; // bit14 ---
                                                        // 1: extarnal clock through XOSC16M_IN PAD into chip, and XOSC16M_OUT PAD is not driven by XOSC16M
                                                        // 0: the XOSC16M_IN & XOSC16M_OUT are connected with crystal oscillator.
        uint32_t PIOA19_FUNC_SEL:                    1; // bit15 ---
                                                        // 0: pioa19 used as reset pin
                                                        // 1: pioa19 used as GPIO
                                                        // can't be reset by pin reset
        uint32_t AONLDO_TRIM_RUN:                    4; // bit[19:16] --- LDOBK_TRIM,
                                                        // inverter the high two bits
                                                        // 0:  1.208
                                                        // 1:  1.253
                                                        // 2:  1.298
                                                        // 3:  1.335
                                                        // 4:  1.035
                                                        // 5:  1.080
                                                        // 6:  1.118
                                                        // 7:  1.163
                                                        // 8:  0.854
                                                        // 9:  0.899
                                                        // 10: 0.944
                                                        // 11: 0.989
                                                        // 12: 0.674
                                                        // 13: 0.719
                                                        // 14: 0.764
                                                        // 15: 0.809
        uint32_t CORELDO_TRIM_RUN:                   5; // bit[24:20] --- CORE LDO trim, used when ST_RUN
        uint32_t RSV_END:                            7; // bit[31:25]
    };
    uint32_t Word;
} AON_BKHOLD_CTRL_TypeDef; //0x008 


//================================
#define AON_RC32K_MSB_TRIM_CFG_LSB          0
#define AON_RC32K_MSB_TRIM_CFG_WIDTH        4
#define AON_RC32K_LSB_TRIM_CFG_LSB          4
#define AON_RC32K_LSB_TRIM_CFG_WIDTH        10
#define AON_XOSC_EST_MODE_POS               14
#define AON_PIOA19_FUNC_SEL_POS             15
#define AON_AONLDO_TRIM_RUN_LSB             16
#define AON_AONLDO_TRIM_RUN_WIDTH           4
#define AON_CORELDO_TRIM_RUN_LSB            20
#define AON_CORELDO_TRIM_RUN_WIDTH          5
//================================

typedef union //0x010 
{
    struct
    {
        uint32_t BLELOWCLK_EN:                       1; // bit0
        uint32_t BLELOWCLK_SEL:                      1; // bit1 --- BLE low power clock source select
                                                        // 0: lsi clock, 32K
                                                        // 1: HSE clock/500; 32K
                                                        // set as 1 condtion:
                                                        // chip is just in deepsleep mode, and makesure
        uint32_t RTC_EN:                             1; // bit2
                                                        // 0: RTC disable
                                                        // 1: RTC enable
        uint32_t RSV_END:                           29; // bit[31:3]
    };
    uint32_t Word;
} AON_BLE_RTC_CTL_TypeDef; //0x010 


//================================
#define AON_BLELOWCLK_EN_POS                0
#define AON_BLELOWCLK_SEL_POS               1
#define AON_RTC_EN_POS                      2
//================================

typedef union //0x014 
{
    struct
    {
        uint32_t SCAN_MODE_REQ:                      1; // bit0
        uint32_t SCAN_ZIP_CFG:                       1; // bit1
        uint32_t RSV_END:                           30; // bit[31:2]
    };
    uint32_t Word;
} AON_SCAN_MODE_CTRL_TypeDef; //0x014 


//================================
#define AON_SCAN_MODE_REQ_POS               0
#define AON_SCAN_ZIP_CFG_POS                1
//================================

typedef union //0x018 
{
    struct
    {
        uint32_t AUTO_DIS_RC32K:                     1; // bit0
                                                        // when auto_dis_rc32k is 1
                                                        //  RC32K is auto disable when chip in poweroff state or deepsleep state
                                                        //  RC32K is auto enable when wakeup event happen for poweroff state or deepsleep
                                                        //  the RTC and BB WAKE LP can't used as wakeup source for DEEPSLEEP or poweroff
                                                        //  only GPIO or reset pin can be use
        uint32_t CORELDO_EN_RUN:                     1; // bit1 --- When PMU is on ST_RUN, CORE LDO enable is controlled by coreldo_en_run
        uint32_t OSC_EN_RUN:                         1; // bit2 --- When PMU is on ST_RUN, OSC enable is controlled by osc_en_run
        uint32_t RC32K_EN_RUN:                       1; // bit3 --- only use when PMU in ST_RUN, RC32K
        uint32_t EM_PD_EN:                           1; // bib4 --- 
                                                        // 1 : EM is power down
                                                        // 0 : EN is power enable
        uint32_t EM_ACCESS_EN:                       1; // bit5 ---
                                                        // 1: EM is can be access
                                                        // 0: EM is isolation
                                                        // note: 
                                                        //   em_access_en set as 0 before em_pd_en set as 1
                                                        //   em_access_en set as 1 after em_pd_en set as 0
        uint32_t RSV_NOUSE1:                         2; // bit[7:6] --- Ignore me
        uint32_t POWEROFF_LOG:                       1; // bit8 ---
                                                        // when is 1, chip is powerup from POWEROFF state
                                                        // else means chip is initial powerup or powerup from BOD event happen
                                                        // cleared by poweroff_log_clr
        uint32_t EM_PD_ACK:                          1; // bit9
        uint32_t RSV_END:                           22; // bit[31:10]
    };
    uint32_t Word;
} AON_PMU_CTRL_TypeDef; //0x018 


//================================
#define AON_AUTO_DIS_RC32K_POS              0
#define AON_CORELDO_EN_RUN_POS              1
#define AON_OSC_EN_RUN_POS                  2
#define AON_RC32K_EN_RUN_POS                3
#define AON_EM_PD_EN_POS                    4
#define AON_EM_ACCESS_EN_POS                5
#define AON_POWEROFF_LOG_POS                8
#define AON_EM_PD_ACK_POS                   9
//================================

typedef union //0x01c 
{
    struct
    {
        uint32_t BLE_WKUP_SEL:                       2; // bit[1:0] --- both use when deepsleep or poweroff state
                                                        // 2'b00 disable ble wakeup
                                                        // 2'b01: select ble_osc_en as wakeup source
                                                        // 2'b10: select ble_radio_en as wakeup source
                                                        // 2'b11: select ble_wakeup_lp as wakeup source
                                                        // when poweroff mode, only ble_osc_en can be used
        uint32_t BLE_LATCH_N:                        1; // bit2 ---
                                                        // 0: latch the signal from BB to BLE LP counter
                                                        // 1: enable the signal from BB to BLE LP counter
                                                        // config as 0 before chip into poweroff mode
                                                        // set as 1 after chip powerup and retain the BB state
        uint32_t IO_WKUP_EN:                         1; // bit3 --- both use when deepsleep or poweroff state
        uint32_t RTC_WKUP_EN:                        1; // bit4 --- both use when deepsleep or poweroff state
        uint32_t IO_LATCH_N:                         1; // bit5 ---
                                                        // 0: latch the GPIO control signal from core to AON
                                                        // 1: enable the GPIO control signal from core to AON
                                                        // config as 0 before chip into poweroff mode
                                                        // set as 1 after chip powerup and retain the GPIO config
        uint32_t RC32K_TRIM_SEL:                     1; // bit6 ---
                                                        // 0: use rc32k_msb_trim_cfg and rc32k_lsb_trim_cfg control the RC32K freq
                                                        // 1: use RC32K calib result control the RC32K freq
        uint32_t RSV_END:                           25; // bit[31:7]
    };
    uint32_t Word;
} AON_PMU_WKUP_CTRL_TypeDef; //0x01c 


//================================
#define AON_BLE_WKUP_SEL_LSB                0
#define AON_BLE_WKUP_SEL_WIDTH              2
#define AON_BLE_LATCH_N_POS                 2
#define AON_IO_WKUP_EN_POS                  3
#define AON_RTC_WKUP_EN_POS                 4
#define AON_IO_LATCH_N_POS                  5
#define AON_RC32K_TRIM_SEL_POS              6
//================================

typedef union //0x020 
{
    struct
    {
        uint32_t IO_WKUP_ST:                         1; // bit0
        uint32_t BLE_WKUP_ST:                        1; // bit1
        uint32_t RTC_WKUP_ST:                        1; // bit2
        uint32_t AON_PMU_INT:                        1; // bit3
        uint32_t RSV_NOUSE1:                         4; // bit[7:4] --- Ignore me
        uint32_t BOD12_OUT12_ST:                     1; // bit8
        uint32_t LVD33_OUT12_ST:                     1; // bit9
        uint32_t RSV_END:                           22; // bit[31:10]
    };
    uint32_t Word;
} AON_PMU_WKUP_ST_TypeDef; //0x020 


//================================
#define AON_IO_WKUP_ST_POS                  0
#define AON_BLE_WKUP_ST_POS                 1
#define AON_RTC_WKUP_ST_POS                 2
#define AON_AON_PMU_INT_POS                 3
#define AON_BOD12_OUT12_ST_POS              8
#define AON_LVD33_OUT12_ST_POS              9
//================================

typedef union //0x024 
{
    struct
    {
        uint32_t BK_BOD_TRIM:                        3; // bit[2:0]
        uint32_t LDO_BOD_EN:                         1; // bit3 --- BOD function:
                                                        // detect the VDD12
                                                        // when VDD12 < vth: BOD12_OUT12 is 1
                                                        // else BOD12_OUT12 is 0
        uint32_t LDO_BOD_RST_EN:                     1; // bit4
        uint32_t LDO12_IBSEL:                        5; // bit[9:5]
        uint32_t LDO_LVD_EN:                         1; // bit10
        uint32_t LDO_LVD_SEL:                        3; // bit[13:11] --- BOD function:
                                                        // detect the VDD33
                                                        // when VDD33 < vth: LVD33_OUT12 is 1
                                                        // else BOD12_OUT12 is 0
        uint32_t LDO_LVD_RST_EN:                     1; // bit14
        uint32_t RSV_NOUSE1:                         1; // bit15      --- Ignore me
        uint32_t ANA_RESV:                          16; // bit[31:16] ---
                                                        // [0]   LDO_DPLL VREG12_EN1 TEST EN
                                                        // [1]   LDO_DPLL VREG12_EN2 TEST EN
                                                        // [2]   LDO_DPLL EN  
                                                        // [3]   LDO32K TEST EN
                                                        // [4]   LDOCORE TEST EN  
                                                        // [7:5] LDODPLL_TRIM[2:0]
                                                        // [8]   DPLL2 ref clock select:0-OSC 16M, 1-RC 16M 
    };
    uint32_t Word;
} AON_PMU_ANA_CTRL_TypeDef; //0x024 


//================================
#define AON_BK_BOD_TRIM_LSB                 0
#define AON_BK_BOD_TRIM_WIDTH               3
#define AON_LDO_BOD_EN_POS                  3
#define AON_LDO_BOD_RST_EN_POS              4
#define AON_LDO12_IBSEL_LSB                 5
#define AON_LDO12_IBSEL_WIDTH               5
#define AON_LDO_LVD_EN_POS                  10
#define AON_LDO_LVD_SEL_LSB                 11
#define AON_LDO_LVD_SEL_WIDTH               3
#define AON_LDO_LVD_RST_EN_POS              14
#define AON_ANA_RESV_LSB                    16
#define AON_ANA_RESV_WIDTH                  16
//================================

typedef union //0x028 
{
    struct
    {
        uint32_t EN_LDO_XOSC_REG:                    1; // bit0
        uint32_t EN_LDO_SEL:                         1; // bit1 --- EN_LDO_XOSC control select
                                                        // 0: EN_LDO_XOSC is controlled by XOSC16M_EN
                                                        // 1: EN_LDO_XOSC is controlled by EN_LDO_XOSC_REG
        uint32_t BYPASS_LDO_XOSC:                    1; // bit2
        uint32_t RSV_END:                           29; // bit[31:3]
    };
    uint32_t Word;
} AON_XOSC16M_CTRL_TypeDef; //0x028 


//================================
#define AON_EN_LDO_XOSC_REG_POS             0
#define AON_EN_LDO_SEL_POS                  1
#define AON_BYPASS_LDO_XOSC_POS             2
//================================

//================================
//BLOCK AON top struct define 
typedef struct
{
    __IO  uint32_t                               BACKUP0             ; // 0x000, valid-bit:16
                                                                       // can't be reset by pin reset, por12_bk_stb 
    __IO  uint32_t                               BACKUP1             ; // 0x004, valid-bit:8
                                                                       // can't be reset by pin reset por12_bk_stb 
    __IO  AON_BKHOLD_CTRL_TypeDef                BKHOLD_CTRL         ; // 0x008, 
                                                                       // can't be reset by pin reset por12_bk_stb 
    __IO  uint32_t                               BACKUP2             ; // 0x00c,  valid-bit:8
                                                                       // can be reset by pin reset  
    __IO  AON_BLE_RTC_CTL_TypeDef                BLE_RTC_CTL         ; // 0x010,  
    __IO  AON_SCAN_MODE_CTRL_TypeDef             SCAN_MODE_CTRL      ; // 0x014, 
                                                                       // how to into scan mode
                                                                       // 1: CPU write this scan_mode_req
                                                                       // 2: wait 200us; 
    __IO  AON_PMU_CTRL_TypeDef                   PMU_CTRL            ; // 0x018,  
    __IO  AON_PMU_WKUP_CTRL_TypeDef              PMU_WKUP_CTRL       ; // 0x01c, 
                                                                       // set deepsleep poweroff wakeup source  
    __I   AON_PMU_WKUP_ST_TypeDef                PMU_WKUP_ST         ; // 0x020,  
    __IO  AON_PMU_ANA_CTRL_TypeDef               PMU_ANA_CTRL        ; // 0x024,  
    __IO  AON_XOSC16M_CTRL_TypeDef               XOSC16M_CTRL        ; // 0x028,  
} AON_TypeDef;


#define AON  (( AON_TypeDef  *)     AON_BASE)

#endif
