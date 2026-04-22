#ifndef _REG_APBMISC_H_
#define _REG_APBMISC_H_

#include "reg_base.h" 

//================================
//BLOCK APBMISC define 

#define APBMISC_BASE                           ((uint32_t)0x40031000) 
#define APBMISC_RC32K_CALIB_ST_ADDR_OFFSET     0x000 
#define APBMISC_RCCALIB_CTRL_ADDR_OFFSET       0x004 
#define APBMISC_RCCALIB_START_ADDR_OFFSET      0x008 
#define APBMISC_RCFREQ_GET_ADDR_OFFSET         0x00c 
#define APBMISC_RC_FREQ_VAL_ADDR_OFFSET        0x010 
#define APBMISC_RCCALIB_STCLR_ADDR_OFFSET      0x014 
#define APBMISC_RC16M_CNT_CTRL_ADDR_OFFSET     0x018 
#define APBMISC_RTCINT_CTRL_ADDR_OFFSET        0x01c 
#define APBMISC_RTC_SEC_ADDR_OFFSET            0x020 
#define APBMISC_RTC_MS_ADDR_OFFSET             0x024 
#define APBMISC_RTC_ALARM_SEC_ADDR_OFFSET      0x028 
#define APBMISC_RTC_ALARM_MS_ADDR_OFFSET       0x02c 
#define APBMISC_RTC_SEC_SHD_ADDR_OFFSET        0x030 
#define APBMISC_RTC_MS_SHD_ADDR_OFFSET         0x034 
#define APBMISC_AON_PMU_CTRL_ADDR_OFFSET       0x038 
#define APBMISC_LDO_UD_CTRL_ADDR_OFFSET        0x03c 
#define APBMISC_PIOA_WKUP_POS_ADDR_OFFSET      0x040 
#define APBMISC_PIOA_WKUP_NEG_ADDR_OFFSET      0x044 
#define APBMISC_POWEROFF_WORD_CMD_ADDR_OFFSET  0x048 
#define APBMISC_POWEROFF_LOG_CLR_ADDR_OFFSET   0x04c 
#define APBMISC_RC16M_FREQ_TRIM_ADDR_OFFSET    0x050 
#define APBMISC_XOSC16M_CTRL_ADDR_OFFSET       0x054 
#define APBMISC_BLE_LP_CTRL_ADDR_OFFSET        0x058 
#define APBMISC_BLE_SYNC_TIME_ADDR_OFFSET      0x05c 
#define APBMISC_BLE_FINECNT_BOUND_ADDR_OFFSET  0x060 
#define APBMISC_BLE_CLKNCNT_SAMP_ADDR_OFFSET   0x064 
#define APBMISC_BLE_FINECNT_SAMP_ADDR_OFFSET   0x068 
#define APBMISC_BLE_CLKNCNT_CFG_ADDR_OFFSET    0x06c 
#define APBMISC_BLE_FINECNT_CFG_ADDR_OFFSET    0x070 
#define APBMISC_BLE_DP_TWCORE_ADDR_OFFSET      0x074 
#define APBMISC_BLE_UPLOAD_ADDR_OFFSET         0x078 
#define APBMISC_DPLL_CTRL_ADDR_OFFSET          0x07c 
#define APBMISC_ANAMISC_CTRL_ADDR_OFFSET       0x080 
#define APBMISC_ANAMISC_ST_ADDR_OFFSET         0x084 

//================================
//BLOCK APBMISC reg struct define 
typedef union //0x000 
{
    struct
    {
        uint32_t RC32K_MSB_CALIB:                    4; // bit[3:0] --- the final RC trim value
        uint32_t RSV_NOUSE1:                        12; // bit[15:4]--- Ignore me
        uint32_t RC32K_LSB_CALIB:                   10; // bit[25:16]
        uint32_t RSV_END:                            6; // bit[31:26]
    };
    uint32_t Word;
} APBMISC_RC32K_CALIB_ST_TypeDef; //0x000 


//================================
#define APBMISC_RC32K_MSB_CALIB_LSB         0
#define APBMISC_RC32K_MSB_CALIB_WIDTH       4
#define APBMISC_RC32K_LSB_CALIB_LSB         16
#define APBMISC_RC32K_LSB_CALIB_WIDTH       10
//================================

typedef union //0x004 
{
    struct
    {
        uint32_t RCCALIB_TARGET:                    20; // bit[19:0] --- rccalib_target = Fref * (rccalib_cycles+1) / Flsi
                                                        // when rccalib_clksel 
                                                        // 2'b00: rccalib_target = 16e6 * rccalib_cycles /32000
                                                        // 2'b01: rccalib_target = 64e6 * rccalib_cycles /32000
                                                        // 2'b10: rccalib_target = 128e6 * rccalib_cycles /32000
        uint32_t RCCALIB_CYCLES:                     8; // bit[27:20] --- each RC calib step window is (rccalib_cycles+1) of LSI clock
        uint32_t RCCALIB_SCAL_EN:                    1; // bit28 ---
                                                        // 0: all calib step window is depend on rccalib_cycles
                                                        // 1: calib step window when search rc32k_msb_calib[3:0] & rc32k_lsb_calib[9:5] is scale down 2
                                                        // and calib step window rc32k_lsb_calib[4:0] is depend on rccalib_cycles
        uint32_t RCCALIB_DLY:                        1; // bit29 ---
                                                        // when is 1:add one LSI clock delay between msb_calib or lsb_calib change to start count
        uint32_t RSV_END:                            2; // bit[31:30]
    };
    uint32_t Word;
} APBMISC_RCCALIB_CTRL_TypeDef; //0x004 


//================================
#define APBMISC_RCCALIB_TARGET_LSB          0
#define APBMISC_RCCALIB_TARGET_WIDTH        20
#define APBMISC_RCCALIB_CYCLES_LSB          20
#define APBMISC_RCCALIB_CYCLES_WIDTH        8
#define APBMISC_RCCALIB_SCAL_EN_POS         28
#define APBMISC_RCCALIB_DLY_POS             29
//================================

typedef union //0x014 
{
    struct
    {
        uint32_t RCCALIB_DONE:                       1; // bit0
        uint32_t RCFREQ_DONE:                        1; // bit1
        uint32_t RSV_NOUSE1:                         6; // bit[7:2] --- Ignore me
        uint32_t RCCALIB_DONE_CLR:                   1; // bit8
        uint32_t RCFREQ_DONE_CLR:                    1; // bit9
        uint32_t RSV_END:                           22; // bit[31:10]
    };
    uint32_t Word;
} APBMISC_RCCALIB_STCLR_TypeDef; //0x014 


//================================
#define APBMISC_RCCALIB_DONE_POS            0
#define APBMISC_RCFREQ_DONE_POS             1
#define APBMISC_RCCALIB_DONE_CLR_POS        8
#define APBMISC_RCFREQ_DONE_CLR_POS         9
//================================

typedef union //0x018 
{
    struct
    {
        uint32_t RC16M_WIN_SET:                     15; // bit[14:0] --- RC16M cnt window
                                                        // (rc16m_win_set<<10) osc16m cycles
        uint32_t RC16M_CNT_START:                    1; // bit15
        uint32_t RC16M_WIN_CNT:                     15; // bit[30:16]
        uint32_t RC16M_CNT_DONE:                     1; // bit31
    };
    uint32_t Word;
} APBMISC_RC16M_CNT_CTRL_TypeDef; //0x018 


//================================
#define APBMISC_RC16M_WIN_SET_LSB           0
#define APBMISC_RC16M_WIN_SET_WIDTH         15
#define APBMISC_RC16M_CNT_START_POS         15
#define APBMISC_RC16M_WIN_CNT_LSB           16
#define APBMISC_RC16M_WIN_CNT_WIDTH         15
#define APBMISC_RC16M_CNT_DONE_POS          31
//================================

typedef union //0x01c 
{
    struct
    {
        uint32_t RTC_INT_EN:                         1; // bit0 --- rtc interrupt enable
        uint32_t RTC_INT_CLR:                        1; // bit1 --- rtc intrrrupt clear
        uint32_t RTC_INT_ST:                         1; // bit2 --- rtc interrupt status
        uint32_t RTC_SET_BUSY:                       1; // bit3
        uint32_t RSV_END:                           28; // bit[31:4]
    };
    uint32_t Word;
} APBMISC_RTCINT_CTRL_TypeDef; //0x01c 


//================================
#define APBMISC_RTC_INT_EN_POS              0
#define APBMISC_RTC_INT_CLR_POS             1
#define APBMISC_RTC_INT_ST_POS              2
#define APBMISC_RTC_SET_BUSY_POS            3
//================================

typedef union //0x038 
{
    struct
    {
        uint32_t LDO_LP_SEL_RUN:                     1; // bit0 ---
                                                        // 0: CORE LDO is HP mode
                                                        // 1: CORE LDO is lp mode
        uint32_t AON_PWR_SEL_RUN:                    1; // bit1 ---
                                                        // 0: select AON_LDO out as AON power
                                                        // 1: select CORE_LDO out as AON power
        uint32_t HSI_EN_RUN:                         1; // bit2 --- When PMU is on ST_RUN, hsi_en is controlled by hsi_en_run
                                                        // 1: RC16M enable
        uint32_t FSH_PW_EN_RUN:                      1; // bit3
        uint32_t RSV_NOUSE1:                         4; // bit[7:4] --- Ignore me
        uint32_t LDO_LP_SEL_DP:                      1; // bit8 --- When PMU is on ST_DP, CORE LDO lp mode is controlled by ldo_lp_sel_dp
        uint32_t AON_PWR_SEL_DP:                     1; // bit9 --- When PMU is on ST_DP, AON_PWR_SEL is controlled by aon_pwr_sel_dp
        uint32_t OSC_EN_DP:                          1; // bit10--- When PMU is on ST_DP, OSC enable is controlled by osc_en_dp
        uint32_t HSI_EN_DP:                          1; // bit11--- When PMU is on ST_DP, hsi_en is controlled by hsi_en_dp
                                                        // 1: RC16M enable
        uint32_t FSH_PW_EN_DP:                       1; // bit12
        uint32_t RSV_NOUSE2:                         3; // bit[15:13] --- Ignore me
        uint32_t CLK_STB_TIME:                       6; // bit[21:16] --- clock stable time for exit from deepsleep
                                                        // unit : 30.5us (RC32K clock period)
                                                        // stable time : 30.5* (clk_stb_time + 1)
                                                        // recommend value: 
                                                        // 6'd3 (120us) for only wait RC16M stable when exit from deepsleep
                                                        // 6'd63(2ms) for both wait RC16M and HOSC16M stable when exit from deepsleep
        uint32_t WKUP_ST_CLR:                        1; // bit22
        uint32_t AON_PMU_INT_CLR:                    1; // bit23
        uint32_t RSV_END:                            8; // bit[31:24]
    };
    uint32_t Word;
} APBMISC_AON_PMU_CTRL_TypeDef; //0x038 


//================================
#define APBMISC_LDO_LP_SEL_RUN_POS          0
#define APBMISC_AON_PWR_SEL_RUN_POS         1
#define APBMISC_HSI_EN_RUN_POS              2
#define APBMISC_FSH_PW_EN_RUN_POS           3
#define APBMISC_LDO_LP_SEL_DP_POS           8
#define APBMISC_AON_PWR_SEL_DP_POS          9
#define APBMISC_OSC_EN_DP_POS               10
#define APBMISC_HSI_EN_DP_POS               11
#define APBMISC_FSH_PW_EN_DP_POS            12
#define APBMISC_CLK_STB_TIME_LSB            16
#define APBMISC_CLK_STB_TIME_WIDTH          6
#define APBMISC_WKUP_ST_CLR_POS             22
#define APBMISC_AON_PMU_INT_CLR_POS         23
//================================

typedef union //0x03c 
{
    struct
    {
        uint32_t CORELDO_TRIM_DP:                    5; // bit[4:0]
        uint32_t RSV_NOUSE1:                         3; // bit[7:5] --- Ignore me
        uint32_t CORELDO_TRIM_STEP:                  5; // bit[12:8]
        uint32_t RSV_NOUSE2:                         3; // bit[15:13]-- Ignore me
        uint32_t AONLDO_TRIM_OFF:                    4; // bit[19:16]
        uint32_t AONLDO_UD_STEP:                     3; // bit[22:20]-- LDOBK TRIM
        uint32_t RSV_END:                            9; // bit[31:23]
    };
    uint32_t Word;
} APBMISC_LDO_UD_CTRL_TypeDef; //0x03c 


//================================
#define APBMISC_CORELDO_TRIM_DP_LSB         0
#define APBMISC_CORELDO_TRIM_DP_WIDTH       5
#define APBMISC_CORELDO_TRIM_STEP_LSB       8
#define APBMISC_CORELDO_TRIM_STEP_WIDTH     5
#define APBMISC_AONLDO_TRIM_OFF_LSB         16
#define APBMISC_AONLDO_TRIM_OFF_WIDTH       4
#define APBMISC_AONLDO_UD_STEP_LSB          20
#define APBMISC_AONLDO_UD_STEP_WIDTH        3
//================================

typedef union //0x054 
{
    struct
    {
        uint32_t XOSC16M_CAP_TR:                     6; // bit[5:0]
        uint32_t XOSC16M_LP:                         1; // bit6
        uint32_t EN_XO16MBUF:                        1; // bit7
        uint32_t XOSC16M_BIAS_ADJ:                   4; // bit[11:8]
        uint32_t XOSC16M_SEL:                        1; // bit12
        uint32_t RSV_NOUSE1:                         1; // bit13 --- Ignore me
        uint32_t LDO_XOSC_TR:                        4; // bit[17:14]
        uint32_t RSV_END:                           14; // bit[31:18]
    };
    uint32_t Word;
} APBMISC_XOSC16M_CTRL_TypeDef; //0x054 


//================================
#define APBMISC_XOSC16M_CAP_TR_LSB          0
#define APBMISC_XOSC16M_CAP_TR_WIDTH        6
#define APBMISC_XOSC16M_LP_POS              6
#define APBMISC_EN_XO16MBUF_POS             7
#define APBMISC_XOSC16M_BIAS_ADJ_LSB        8
#define APBMISC_XOSC16M_BIAS_ADJ_WIDTH      4
#define APBMISC_XOSC16M_SEL_POS             12
#define APBMISC_LDO_XOSC_TR_LSB             14
#define APBMISC_LDO_XOSC_TR_WIDTH           4
//================================

typedef union //0x058 
{
    struct
    {
        uint32_t BLE_WAKEUP_REQ:                     1; // bit0     --- software wakeup the BB from BB in deepsleep
        uint32_t RSV_NOUSE1:                         7; // bit[7:1] --- Ignore me
        uint32_t BLE_OSC_EN_ST:                      1; // bit8
        uint32_t BLE_RADIO_EN_ST:                    1; // bit9
        uint32_t BLE_SAMP_VALID:                     1; // bit10 ---
                                                        // when is 1, ble_clkncnt_samp & ble_finecnt_samp can be read
                                                        // auto cleared by HW
        uint32_t BLE_DP_VALID:                       1; // bit11 ---
                                                        // when is 1, BB is deepsleep, and the remained sleep time is larger than ble_dp_twcore
                                                        // auto cleared when BB exit deepsleep
        uint32_t RSV_END:                           20; // bit[31:12]
    };
    uint32_t Word;
} APBMISC_BLE_LP_CTRL_TypeDef; //0x058 


//================================
#define APBMISC_BLE_WAKEUP_REQ_POS          0
#define APBMISC_BLE_OSC_EN_ST_POS           8
#define APBMISC_BLE_RADIO_EN_ST_POS         9
#define APBMISC_BLE_SAMP_VALID_POS          10
#define APBMISC_BLE_DP_VALID_POS            11
//================================

typedef union //0x07c
{
    struct
    {
        uint32_t RSV_NOUSE1:                         2; // bit[1:0] --- Ignore me
        uint32_t DPLL2_EN:                           1; // bit2
        uint32_t DPLL2_LOCK_BYPASS_EN:               1; // bit3
        uint32_t DPLL2_EN_CLK48M:                    1; // bit4
        uint32_t RSV_END:                           27; // bit[31:5]
    };
  uint32_t Word;
} APBMISC_DPLL_CTRL_TypeDef; //0x07c 


//================================
#define APBMISC_DPLL2_EN_POS                2
#define APBMISC_DPLL2_LOCK_BYPASS_EN_POS    3
#define APBMISC_DPLL2_EN_CLK48M_POS         4
//================================

typedef union //0x078 
{
    struct
    {
        uint32_t TESTA_CONN2XO_EN:                   1; // bit0
        uint32_t LDO_BOD_TEST_EN:                    1; // bit1
        uint32_t LDO_BOD_INT_EN:                     1; // bit2
        uint32_t LDO_LVD_INT_EN:                     1; // bit3
        uint32_t BK_VBK_TEST_EN:                     1; // bit4
        uint32_t RSV_END:                           27; // bit[31:5]
    };
    uint32_t Word;
} APBMISC_ANAMISC_CTRL_TypeDef; //0x078 


//================================
#define APBMISC_TESTA_CONN2XO_EN_POS        0
#define APBMISC_LDO_BOD_TEST_EN_POS         1
#define APBMISC_LDO_BOD_INT_EN_POS          2
#define APBMISC_LDO_LVD_INT_EN_POS          3
#define APBMISC_BK_VBK_TEST_EN_POS          4
//================================

typedef union //0x084 
{
    struct
    {
        uint32_t RSV_NOUSE1:                         1; // bit0 ---Ignore me
        uint32_t DPLL2_LOCK_READY:                   1; // bit1
        uint32_t LVD_MODE:                           1; // bit2 ---
                                                        // 0: 1.8V IO voltage
                                                        // 1: 3.3V IO valtage
        uint32_t RSV_END:                           29; // bit[31:3]
    };
    uint32_t Word;
} APBMISC_ANAMISC_ST_TypeDef; //0x07c 


//================================
#define APBMISC_DPLL2_LOCK_READY_POS        1
#define APBMISC_LVD_MODE_POS                2
//================================

//================================
//BLOCK APBMISC top struct define 
typedef struct
{
    __I   APBMISC_RC32K_CALIB_ST_TypeDef         RC32K_CALIB_ST      ; // 0x000,  
    __IO  APBMISC_RCCALIB_CTRL_TypeDef           RCCALIB_CTRL        ; // 0x004,  
    __O   uint32_t                               RCCALIB_START       ; // 0x008, 
                                                                       // RC32K calib follow
                                                                       // 0: set AON.rc32k_trim_sel =1
                                                                       // 1: select calib ref clock: RCC.rccalib_clksel, enable calib clock : RCC.rccalib_clken=1
                                                                       // 2: config rccalib_target and rccalib_cycles, set rccalib_start as 1
                                                                       // 3: wait rccalib_done as 1
                                                                       // 4: set rccalib_done_clr as 1 to clear status
                                                                       // 5: read rc32k_msb_calib and rc32k_lsb_calib
                                                                       // 6: write AON.rc32k_msb_trim_cfg and AON.rc32k_lsb_trim_cfg
                                                                       // 7: set AON.rc32k_trim_sel =0 
    __O   uint32_t                               RCFREQ_GET          ; // 0x00c, 
                                                                       // 0: config rccalib_target and rccalib_cycles, set rcfreq_get as 1
                                                                       // 1: wait rcfreq_done as 1
                                                                       // 2: set rcfreq_done_clr as 1 to clear status
                                                                       // 3: read rc_freq_val 
    __I   uint32_t                               RC_FREQ_VAL         ; // 0x010,  
    __IO  APBMISC_RCCALIB_STCLR_TypeDef          RCCALIB_STCLR       ; // 0x014,  
    __IO  APBMISC_RC16M_CNT_CTRL_TypeDef         RC16M_CNT_CTRL      ; // 0x018,  
    __IO  APBMISC_RTCINT_CTRL_TypeDef            RTCINT_CTRL         ; // 0x01c,  
    __IO  uint32_t                               RTC_SEC             ; // 0x020, 
                                                                       // direct set current RTC value 
    __IO  uint32_t                               RTC_MS              ; // 0x024, 
                                                                       // direct set current RTC value
                                                                       // note rtc_sec is must set before rtc_ms 
    __IO  uint32_t                               RTC_ALARM_SEC       ; // 0x028,  
    __IO  uint32_t                               RTC_ALARM_MS        ; // 0x02c,  
    __I   uint32_t                               RTC_SEC_SHD         ; // 0x030,  
    __I   uint32_t                               RTC_MS_SHD          ; // 0x034,  
    __IO  APBMISC_AON_PMU_CTRL_TypeDef           AON_PMU_CTRL        ; // 0x038,  
    __IO  APBMISC_LDO_UD_CTRL_TypeDef            LDO_UD_CTRL         ; // 0x03c,  
    __IO  uint32_t                               PIOA_WKUP_POS       ; // 0x040, 
                                                                       // enable the IO posedge wakeup the chip from deepsleep or poweroff
                                                                       // IO0 ~ IO18 
    __IO  uint32_t                               PIOA_WKUP_NEG       ; // 0x044, 
                                                                       // enable the IO negedge wakeup the chip from deepsleep or poweroff
                                                                       // IO0 ~ IO19 
    __O   uint32_t                               POWEROFF_WORD_CMD   ; // 0x048, 
                                                                       // software POWEROFF cmd
                                                                       // when poweroff_word_cmd[31:0] is written as 32'h77ED_29B4, chip go to POWEROFF 
    __O   uint32_t                               POWEROFF_LOG_CLR    ; // 0x04c,  
    __IO  uint32_t                               RC16M_FREQ_TRIM     ; // 0x050, 
                                                                       // RC16M trim reg 
    __IO  APBMISC_XOSC16M_CTRL_TypeDef           XOSC16M_CTRL        ; // 0x054,  
    __IO  APBMISC_BLE_LP_CTRL_TypeDef            BLE_LP_CTRL         ; // 0x058,  
    __I   uint32_t                               BLE_SYNC_TIME       ; // 0x05c, 
                                                                       // the time from pos of sync_window to syncfound
                                                                       // unit : 16M clock period
                                                                       // ble_sync_time[19:4] is us  
    __IO  uint32_t                               BLE_FINECNT_BOUND   ; // 0x060, 
                                                                       // finecnt boundary
                                                                       // set the  half slot time : ( ble_finecnt_bound + 1 ) *0.5 us
                                                                       // unit : 0.5us
                                                                       // default is 312.5us ( BLE half slot time) 
    __I   uint32_t                               BLE_CLKNCNT_SAMP    ; // 0x064,  
    __I   uint32_t                               BLE_FINECNT_SAMP    ; // 0x068,  
    __O   uint32_t                               BLE_CLKNCNT_CFG     ; // 0x06c,  
    __O   uint32_t                               BLE_FINECNT_CFG     ; // 0x070,  
    __IO  uint32_t                               BLE_DP_TWCORE       ; // 0x074, 
                                                                       // the threhold for BB remained deepsleep time for Core go into Deepsleep
                                                                       // unit: RC32K clock
                                                                       // default is 5ms 
    __O   uint32_t                               BLE_UPLOAD          ; // 0x078, 
                                                                       // set as 1 when chip exit from poweroff and write back the BB reg 
    __IO  APBMISC_DPLL_CTRL_TypeDef              DPLL_CTRL           ; // 0x07c,  
    __IO  APBMISC_ANAMISC_CTRL_TypeDef           ANAMISC_CTRL        ; // 0x080,  
    __I   APBMISC_ANAMISC_ST_TypeDef             ANAMISC_ST          ; // 0x084,  
} APBMISC_TypeDef;


#define APBMISC  (( APBMISC_TypeDef  *)     APBMISC_BASE)

#endif
