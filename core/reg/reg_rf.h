#ifndef _REG_RF_H_
#define _REG_RF_H_

#include "reg_base.h" 

//================================
//BLOCK RF define 

#define RF_BASE                                ((uint32_t)0x4002C000) 
#define RF_DIG_CTRL_ADDR_OFFSET                0x000 
#define RF_PLL_TAB_OFFSET_ADDR_OFFSET          0x004 
#define RF_PLL_GAIN_CTRL_ADDR_OFFSET           0x008 
#define RF_RSSI_CTRL_ADDR_OFFSET               0x00c 
#define RF_PLL_DAC_TAB0_ADDR_OFFSET            0x010 
#define RF_PLL_CAL_ST_ADDR_OFFSET              0x014 
#define RF_PLL_FREQ_CTRL_ADDR_OFFSET           0x018 
#define RF_PLL_DYM_CTRL_ADDR_OFFSET            0x01c 
#define RF_FSM_DLY_CTRL0_ADDR_OFFSET           0x020 
#define RF_FSM_DLY_CTRL1_ADDR_OFFSET           0x024 
#define RF_PLL_CAL_DAC_STEP_ADDR_OFFSET        0x028 
#define RF_PLL_GAIN_CAL_VAL_ADDR_OFFSET        0x02c 
#define RF_PLL_DAC_TAB1_ADDR_OFFSET            0x030 
#define RF_PLL_DAC_TAB2_ADDR_OFFSET            0x034 
#define RF_DATA_DLY_CTRL_ADDR_OFFSET           0x038 
#define RF_TX_CH_MAP_ADDR_OFFSET               0x03c 
#define RF_RX_CHIDX37_MAP_ADDR_OFFSET          0x040 
#define RF_RX_CHIDX38_MAP_ADDR_OFFSET          0x044 
#define RF_RX_CHIDX39_MAP_ADDR_OFFSET          0x048 
#define RF_BPFMIX_CTRL_ADDR_OFFSET             0x04c 
#define RF_AGC_CTRL0_ADDR_OFFSET               0x050 
#define RF_AGC_CTRL1_ADDR_OFFSET               0x054 
#define RF_AGC_CTRL2_ADDR_OFFSET               0x058 
#define RF_ANA_TRIM_ADDR_OFFSET                0x05c 
#define RF_ANAMISC_CTRL1_ADDR_OFFSET           0x060 
#define RF_ANA_PWR_CTRL_ADDR_OFFSET            0x064 
#define RF_ANA_EN_CTRL_ADDR_OFFSET             0x068 
#define RF_PLL_ANA_CTRL_ADDR_OFFSET            0x06c 
#define RF_RF_RSV_ADDR_OFFSET                  0x070 
#define RF_RF_ANA_ST0_ADDR_OFFSET              0x074 

//================================
//BLOCK RF reg struct define 
typedef union //0x000 
{
    struct
    {
        uint32_t ANA_TEST_EN:                        1; // bit0 ---
                                                        // 0: RF ANALOG digital input is controlled by rfdig_gsm
                                                        // 1: RF ANALOG digital input is controlled by reg
        uint32_t LDO_TEST_EN:                        1; // bit1 ---
                                                        // 0: analog block power is controlled by RF Dig FSM
                                                        // 1: analog block power is controlled by reg
        uint32_t PLL_CAL_TEST_EN:                    1; // bit2 ---
                                                        // 0: PLL AFC & GAIN CAL controlled by rf_pll_cal module
                                                        // 1: PLL AFC & GAIN CAL directed controlled by reg
        uint32_t FSM_CTRL_SEL:                       1; // bit3 ---
                                                        // 0: RF Dig FSM input signal is from BB 
                                                        // 1: RF Dig FSM input signal is from rf_reg
                                                        //  : channel number
                                                        //  : rate 
                                                        //  : tx_en
                                                        //  : rx_en
        uint32_t PA_GAIN_TARGET_SEL:                 1; // bit4 ---
                                                        // 0: the pa_gain_target is from BB
                                                        // 1: the pa_gain_target is from sw_pa_gain_target
        uint32_t PLL_AFC_BY:                         1; // bit5 ---
                                                        // 0: rfdig FSM start AFC calib
                                                        // 1: rfdig FSM bypass AFC calib,
        uint32_t PLL_BPF_CAL_BY:                     1; // bit6 ---
                                                        // 0: rfdig FSM start BPF calib
                                                        // 1: rfdig FSM bypass BPF calib, 
        uint32_t PLL_FREQ_SEL:                       1; // bit7 ---
                                                        // 0: PLL frequency is controlled by channel number 
                                                        //    PLL_RTX_SEL is controlled by  PLL_RTX_SEL_REG
                                                        // 1: PLL frequency is controlled by reg sw_di_s & sw_frac
                                                        //    PLL_RTX_SEL is controlled by  fsm_pll_rtx_sel 
        uint32_t PLL_FREQ_EXT_EN:                    1; // bit8 ---
                                                        // 0: PLL_FREQ_ADJ[4:0] is driven by AFC Calib logic 
                                                        // 1: PLL_FREQ_ADJ[4:0] is driven by PLL_FREQ_EXT
        uint32_t PLL_GAIN_CAL_BY:                    1; // bit9 ---
                                                        // 0: rfdig FSM start PLL GAIN calib
                                                        // 1: rfdig FSM bypass PLL GAIN calib
        uint32_t PLL_VTXD_EXT_EN:                    1; // bit10 ---
                                                        // 0: PLL_DAC_ADJ[4:0] is driven by GAIN Calib logic 
                                                        // 1: PLL_DAC_ADJ[4:0] is driven by PLL_VTXD_EXT
        uint32_t PLL_GAIN_CAL_STEP:                  1; // bit11 ---
                                                        // 1: MCU step by step debug PLL GAIN CAL
                                                        // note: only effect when pll_gain_cal_by is 1
        uint32_t PLL_GAIN_CAL_TAB:                   1; // bit12 ---
                                                        // active when pll_gain_cal_by is 1 and pll_vtxd_ext_en is 0 & pll_gain_cal_step is 0
                                                        // 1: PLL_DAC_ADJ[4:0] is driven by lookup table
        uint32_t PLL_GAIN_CAL_MODE:                  2; // bit[14:13] ---
                                                        // 0,1 : dividor 2 , calib start from 5'h10
                                                        // 2: use the lookup table out, and detect the df1 error
                                                        //    with pll_gain_cal_th, when df1 error is too larger than pll_gain_cal_dac_step
                                                        //    just report pll_gain_cal_err state; 
                                                        //    the pll_gain_cal_err state is cleared when next time of tx_en is 1, or 
                                                        // 3: use the lookup table out as gain cal start value, and then detect the df1 err
                                                        //    the final calib out = table_out +  df1 err / pll_gain_cal_dac_step
        uint32_t PLL_AFC_ROUND:                      1; // bit15 ---
                                                        // 0: floor result 
        uint32_t PLL_AFC_STEP:                       1; // bit16 ---
                                                        // 1: MCU step by step debug PLL AFC    
                                                        // note: only effect when pll_afc_by is 1
        uint32_t PLL_FREQ_DC:                        3; // bit[19:17] ---
                                                        // DC offset used for AFC calib done
                                                        // signed data: -4 ~ 3
                                                        // 1: round result 
        uint32_t PLL_AFC_MODE:                       1; // bit20 ---
                                                        // 0: set hbw_en as 1 when AFC is done
                                                        // 1: don't set hbw_en as 1 when AFC is done
                                                        // note: set as 1 before makesure pll_gain_cal_by is 0
        uint32_t PLL_AFC_FRAC_EN:                    1; // bit21 ---
                                                        // 1: use PLL_FRAC[23] 
        uint32_t SW_PLL_CAL_CLKEN:                   1; // bit22 ---
                                                        // enable PLL AFC & GAIN CAL clock by MCU
                                                        // set as 1 when pll_afc_by or pll_gain_cal_by is 1
        uint32_t SW_AFC_EN:                          1; // bit23 ---
                                                        // 1:  PLL AFC Calibration start work
                                                        // note: only effect when pll_afc_by is 1
        uint32_t SW_GAIN_CAL_EN:                     1; // bit24 ---
                                                        // 1: PLL GAIN Calibration start work
                                                        // note: only effect when pll_gain_cal_by is 1
        uint32_t SW_PLL_VREF_SEL:                    1; // bit25 ---
                                                        // use to control DAC_VREF_SEL when pll_gain_cal_step is 1
                                                        // or used when pll_cal_test_en is 1
        uint32_t SW_PLL_HBW_EN:                      1; // bit26 ---
                                                        // 0: HBW_EN is driven by pll_cal logic
                                                        // 1: software set PLL is in HBW_EN as 1
        uint32_t SDM_FRAC_INT_MODE:                  1; // bit27 ---
        uint32_t PLL_CAL_ERR_CLR:                    1; // bit28 ---
        uint32_t CAL_ERR_INT_EN:                     1; // bit29 ---
                                                        // rf_int = cal_err_int_en & pll_gain_cal_err
        uint32_t RF_DBG_SEL:                         2; // bit[31:30] ---
                                                        // 2'b01: tx debug port
                                                        // 2'b10: rx debug port
    };
    uint32_t Word;
} RF_DIG_CTRL_TypeDef; //0x000 


//================================
#define RF_ANA_TEST_EN_POS                  0
#define RF_LDO_TEST_EN_POS                  1
#define RF_PLL_CAL_TEST_EN_POS              2
#define RF_FSM_CTRL_SEL_POS                 3
#define RF_PA_GAIN_TARGET_SEL_POS           4
#define RF_PLL_AFC_BY_POS                   5
#define RF_PLL_BPF_CAL_BY_POS               6
#define RF_PLL_FREQ_SEL_POS                 7
#define RF_PLL_FREQ_EXT_EN_POS              8
#define RF_PLL_GAIN_CAL_BY_POS              9
#define RF_PLL_VTXD_EXT_EN_POS              10
#define RF_PLL_GAIN_CAL_STEP_POS            11
#define RF_PLL_GAIN_CAL_TAB_POS             12
#define RF_PLL_GAIN_CAL_MODE_LSB            13
#define RF_PLL_GAIN_CAL_MODE_WIDTH          2
#define RF_PLL_AFC_ROUND_POS                15
#define RF_PLL_AFC_STEP_POS                 16
#define RF_PLL_FREQ_DC_LSB                  17
#define RF_PLL_FREQ_DC_WIDTH                3
#define RF_PLL_AFC_MODE_POS                 20
#define RF_PLL_AFC_FRAC_EN_POS              21
#define RF_SW_PLL_CAL_CLKEN_POS             22
#define RF_SW_AFC_EN_POS                    23
#define RF_SW_GAIN_CAL_EN_POS               24
#define RF_SW_PLL_VREF_SEL_POS              25
#define RF_SW_PLL_HBW_EN_POS                26
#define RF_SDM_FRAC_INT_MODE_POS            27
#define RF_PLL_CAL_ERR_CLR_POS              28
#define RF_CAL_ERR_INT_EN_POS               29
#define RF_RF_DBG_SEL_LSB                   30
#define RF_RF_DBG_SEL_WIDTH                 2
//================================

typedef union //0x004 
{
    struct
    {
        uint32_t TX_FRAC_OFFSET:                    10; // bit[9:0] ---
                                                        // signed data, unit : 1MHz/1024, near as 1KHz
                                                        // arrange: -512KHz ~ 511 KHz
        uint32_t RX_FRAC_OFFSET:                    10; // bit[19:10] ---
                                                        // signed data, unit : 1MHz/1024, near as 1KHz
                                                        // arrange: -512KHz ~ 511 KHz
        uint32_t PLL_FREQ_EXT:                       6; // bit[25:20] ---
                                                        // used when pll_freq_ext_en or pll_afc_step is 1
        uint32_t RSV_END:                            6; // bit[31:26]
    };
    uint32_t Word;
} RF_PLL_TAB_OFFSET_TypeDef; //0x004 


//================================
#define RF_TX_FRAC_OFFSET_LSB               0
#define RF_TX_FRAC_OFFSET_WIDTH             10
#define RF_RX_FRAC_OFFSET_LSB               10
#define RF_RX_FRAC_OFFSET_WIDTH             10
#define RF_PLL_FREQ_EXT_LSB                 20
#define RF_PLL_FREQ_EXT_WIDTH               6
//================================

typedef union //0x008 
{
    struct
    {
        uint32_t PLL_VTXD_EXT:                       6; // bit[5:0] ---
                                                        // used when pll_vtxd_ext_en or pll_gain_cal_step is 1
        uint32_t PLL_GAIN_CAL_DC:                    3; // bit[8:6] ---
                                                        // DC offset used for GAIN CAL calib done
                                                        // signed data: -4 ~ 3
        uint32_t PLL_GAIN_CAL_WIN:                   6; // bit[14:9] ---
                                                        // PLL GAIN CAL step window
                                                        // step window time : (pll_gain_cal_win + 1)  us
        uint32_t PLL_GAIN_CAL_TH:                   12; // bit[26:15] ---
                                                        // gain cal clock : afc_clk = clk_vco/ 4
                                                        // df1 target value is 250K, peak to peak is 2 * df1
                                                        // K is default 80 :
                                                        // 2* df1*K /4: 2* 250K * 80 /4 = 10M
                                                        // pll_gain_cal_th : 10 * (GAIN CAL step window)
        uint32_t RSV_END:                            5; // bit[31:27]
    };
    uint32_t Word;
} RF_PLL_GAIN_CTRL_TypeDef; //0x008 


//================================
#define RF_PLL_VTXD_EXT_LSB                 0
#define RF_PLL_VTXD_EXT_WIDTH               6
#define RF_PLL_GAIN_CAL_DC_LSB              6
#define RF_PLL_GAIN_CAL_DC_WIDTH            3
#define RF_PLL_GAIN_CAL_WIN_LSB             9
#define RF_PLL_GAIN_CAL_WIN_WIDTH           6
#define RF_PLL_GAIN_CAL_TH_LSB              15
#define RF_PLL_GAIN_CAL_TH_WIDTH            12
//================================

typedef union //0x00c 
{
    struct
    {
        uint32_t RSSI_VAL:                           8; // bit[7:0]
        uint32_t AGC_LNA_GAIN:                       4; // bit[11:9]
        uint32_t AGC_BPF_GAIN_ADJ:                   2; // bit[13:12]
        uint32_t AGC_MIXL_GAIN_CTL:                  1; // bit14
        uint32_t AGC_MIXH_GAIN_CTL:                  1; // bit15
        uint32_t SW_RSSI_READY:                      1; // bit16
        uint32_t SW_RSSI_REQ:                        1; // bit17
        uint32_t RF_FSM_STATE:                       5; // bit[22:18]
        uint32_t RSV_END:                            9; // bit[31:23]
    };
    uint32_t Word;
} RF_RSSI_CTRL_TypeDef; //0x00c 


//================================
#define RF_RSSI_VAL_LSB                     0
#define RF_RSSI_VAL_WIDTH                   8
#define RF_AGC_LNA_GAIN_LSB                 8
#define RF_AGC_LNA_GAIN_WIDTH               4
#define RF_AGC_BPF_GAIN_ADJ_LSB             12
#define RF_AGC_BPF_GAIN_ADJ_WIDTH           2
#define RF_AGC_MIXL_GAIN_CTL_POS            14
#define RF_AGC_MIXH_GAIN_CTL_POS            15
#define RF_SW_RSSI_READY_POS                16
#define RF_SW_RSSI_REQ_POS                  17
#define RF_RF_FSM_STATE_LSB                 18
#define RF_RF_FSM_STATE_WIDTH               5
//================================

typedef union //0x010 
{
    struct
    {
        uint32_t PLL_DAC_ADJ00:                      6; // bit[5:0]   ---
                                                        // 2402 ~ 2407
        uint32_t PLL_DAC_ADJ01:                      6; // bit[11:6]  ---
                                                        // 2416 ~ 2423
        uint32_t PLL_DAC_ADJ02:                      6; // bit[17:12] ---
                                                        // 2432 ~ 2439
        uint32_t PLL_DAC_ADJ03:                      6; // bit[23:18] ---
                                                        // 2448 ~ 2455
        uint32_t PLL_DAC_ADJ04:                      6; // bit[29:24] ---
                                                        // 2464 ~ 2471
        uint32_t RSV_END:                            2; // bit[31:30]
    };
    uint32_t Word;
} RF_PLL_DAC_TAB0_TypeDef; //0x010 


//================================
#define RF_PLL_DAC_ADJ00_LSB                0
#define RF_PLL_DAC_ADJ00_WIDTH              6
#define RF_PLL_DAC_ADJ01_LSB                6
#define RF_PLL_DAC_ADJ01_WIDTH              6
#define RF_PLL_DAC_ADJ02_LSB                12
#define RF_PLL_DAC_ADJ02_WIDTH              6
#define RF_PLL_DAC_ADJ03_LSB                18
#define RF_PLL_DAC_ADJ03_WIDTH              6
#define RF_PLL_DAC_ADJ04_LSB                24
#define RF_PLL_DAC_ADJ04_WIDTH              6
//================================

typedef union //0x014 
{
    struct
    {
        uint32_t AFC_CALIB_DONE:                     1; // bit0
        uint32_t GAIN_CALIB_DONE:                    1; // bit1
        uint32_t PLL_FREQ_ADJ_ST:                    6; // bit[7:2]
        uint32_t PLL_GAIN_CAL_ST:                    6; // bit[13:8]
        uint32_t CALIB_CNT_RPT:                     16; // bit[29:14]
        uint32_t PLL_GAIN_CAL_ERR:                   1; // bit30 ---
                                                        // when pll_gain_cal_mode is 2'b10
                                                        // set as 1 when the df1 err is too larger than pll_gain_cal_dac_step
                                                        // ceared by next tx_en is 1 or pll_cal_err_clr is 1
        uint32_t RSV_END:                            1; // bit31
    };
    uint32_t Word;
} RF_PLL_CAL_ST_TypeDef; //0x014 


//================================
#define RF_AFC_CALIB_DONE_POS               0
#define RF_GAIN_CALIB_DONE_POS              1
#define RF_PLL_FREQ_ADJ_ST_LSB              2
#define RF_PLL_FREQ_ADJ_ST_WIDTH            6
#define RF_PLL_GAIN_CAL_ST_LSB              8
#define RF_PLL_GAIN_CAL_ST_WIDTH            6
#define RF_CALIB_CNT_RPT_LSB                14
#define RF_CALIB_CNT_RPT_WIDTH              16
#define RF_PLL_GAIN_CAL_ERR_POS             30
//================================

typedef union //0x018 
{
    struct
    {
        uint32_t SW_PLL_FRAC:                       24; // bit[23:0]
        uint32_t SW_PLL_DI_S:                        5; // bit[28:24]
        uint32_t RSV_END:                            3; // bit[31:29]
    };
    uint32_t Word;
} RF_PLL_FREQ_CTRL_TypeDef; //0x018 


//================================
#define RF_SW_PLL_FRAC_LSB                  0
#define RF_SW_PLL_FRAC_WIDTH                24
#define RF_SW_PLL_DI_S_LSB                  24
#define RF_SW_PLL_DI_S_WIDTH                5
//================================

typedef union //0x01c 
{
    struct
    {
        uint32_t SW_CH_NUM:                          6; // bit[5:0] ---
                                                        // only used when FSM_CTRL_SEL is 1
        uint32_t SW_RATE:                            2; // bit[7:6] ---
                                                        // only used when FSM_CTRL_SEL is 1
                                                        // 00: 1M
                                                        // 01: 2M
                                                        // 1x: forbidden
        uint32_t SW_TX_EN:                           1; // bit8 ---
                                                        // only used when FSM_CTRL_SEL is 1
        uint32_t SW_RX_EN:                           1; // bit9 ---
                                                        // only used when FSM_CTRL_SEL is 1
        uint32_t SW_PA_GAIN_TARGET:                  4; // bit[13:10] ---
                                                        // when ana_test is 1
                                                        // PA_GAIN is driven by pa_gain_target
                                                        // else PA_GAIN is driven by RF Dig FSM, and ramp up to pa_gain_target 
        uint32_t RSV_END:                           18; // bit[31:14]
    };
    uint32_t Word;
} RF_PLL_DYM_CTRL_TypeDef; //0x01c 


//================================
#define RF_SW_CH_NUM_LSB                    0
#define RF_SW_CH_NUM_WIDTH                  6
#define RF_SW_RATE_LSB                      6
#define RF_SW_RATE_WIDTH                    2
#define RF_SW_TX_EN_POS                     8
#define RF_SW_RX_EN_POS                     9
#define RF_SW_PA_GAIN_TARGET_LSB            10
#define RF_SW_PA_GAIN_TARGET_WIDTH          4
//================================

typedef union //0x020 
{
    struct
    {
        uint32_t LDO_START_TIME:                     4; // bit[3:0] ---
                                                        // wait RF LDO stable time (ldo_start_time + 1) us , and then select PLL_RTX_SEL value
                                                        // unit is us
                                                        // 2~10 us, default is 3us
        uint32_t EN_PLL_TIME:                        4; // bit[7:4] ---
                                                        // after LDO stable, delay (en_pll_time + 1) us, then enable PLL; unit : us  
                                                        // 2~10 us, default is 3us
        uint32_t AFC_START_TIME:                     4; // bit[11:8] ---
                                                        // delay (afc_start_time +1) us to enable AFC after pll enable,
                                                        // 2~10 us, default is 3us
        uint32_t BPF_START_TIME:                     6; // bit[17:12] ---
                                                        // delay (bpf_start_time + 1) us to enable BPF CAL after pll enable
        uint32_t TX_END_DLY_TIME:                    4; // bit[21:18] ---
                                                        // delay (tx_end_dly_time + 1) us from neg of bb_tx_en to neg modem_tx_en
        uint32_t RX_END_DLY_TIME:                    4; // bit[25:22] ---
                                                        // delay (rx_end_dly_time + 1) us from neg of bb_rx_en to neg modem_rx_en
        uint32_t TX_PA_RAMPDOWN_TIME:                4; // bit[29:26] ---
                                                        // delay (tx_pa_rampdown_time+1) us to start PA RAMPDOWN after neg of modem_tx_en
        uint32_t RSV_END:                            2; // bit[31:30]
    };
    uint32_t Word;
} RF_FSM_DLY_CTRL0_TypeDef; //0x020 


//================================
#define RF_LDO_START_TIME_LSB               0
#define RF_LDO_START_TIME_WIDTH             4
#define RF_EN_PLL_TIME_LSB                  4
#define RF_EN_PLL_TIME_WIDTH                4
#define RF_AFC_START_TIME_LSB               8
#define RF_AFC_START_TIME_WIDTH             4
#define RF_BPF_START_TIME_LSB               12
#define RF_BPF_START_TIME_WIDTH             6
#define RF_TX_END_DLY_TIME_LSB              18
#define RF_TX_END_DLY_TIME_WIDTH            4
#define RF_RX_END_DLY_TIME_LSB              22
#define RF_RX_END_DLY_TIME_WIDTH            4
#define RF_TX_PA_RAMPDOWN_TIME_LSB          26
#define RF_TX_PA_RAMPDOWN_TIME_WIDTH        4
//================================

typedef union //0x024 
{
    struct
    {
        uint32_t LNA_START_TIME:                     8; // bit[7:0] ---
                                                        // delay (lna_start_time +1) us to enable 
                                                        // LNA, MIX, BPF, LMT, RSSI
                                                        // after pll enable
        uint32_t EN_PA_TIME:                         8; // bit[15:8] ---
                                                        // delay (enpa_time +1) us to enable PA_GAIN after tx enable
                                                        // unit : us
                                                        // Note: make sure en_pa_time is larger than afc_start_time 
        uint32_t AGC_START_TIME:                     4; // bit[19:16] ---
                                                        // delay (agc_start_time +1) us to enable AGC after enable LNA & MIXER
        uint32_t PA_STARTUP_TIME:                    4; // bit[23:20] ---
                                                        // start PA rampup time after PA GAIN enable
                                                        // unit : us   
        uint32_t PA_STEP_TIME:                       4; // bit[27:24] ---
                                                        // PA GAIN step time, unit : 0.5us
                                                        // step time : 0.5* ( pa_step_time + 1 ) us
                                                        // 0.5 ~ 8us
                                                        // 0: is forbidden
        uint32_t RSV_END:                            4; // bit[31:28]
    };
    uint32_t Word;
} RF_FSM_DLY_CTRL1_TypeDef; //0x024 


//================================
#define RF_LNA_START_TIME_LSB               0
#define RF_LNA_START_TIME_WIDTH             8
#define RF_EN_PA_TIME_LSB                   8
#define RF_EN_PA_TIME_WIDTH                 8
#define RF_AGC_START_TIME_LSB               16
#define RF_AGC_START_TIME_WIDTH             4
#define RF_PA_STARTUP_TIME_LSB              20
#define RF_PA_STARTUP_TIME_WIDTH            4
#define RF_PA_STEP_TIME_LSB                 24
#define RF_PA_STEP_TIME_WIDTH               4
//================================

typedef union //0x02c 
{
    struct
    {
        uint32_t PLL_GAIN_CAL_DF0:                  12; // bit[11:0]
        uint32_t PLL_GAIN_CAL_DF1:                  12; // bit[23:12]
        uint32_t PLL_CAL_STATE:                      5; // bit[28:24]
        uint32_t RSV_END:                            3; // bit[31:29]
    };
    uint32_t Word;
} RF_PLL_GAIN_CAL_VAL_TypeDef; //0x02c 


//================================
#define RF_PLL_GAIN_CAL_DF0_LSB             0
#define RF_PLL_GAIN_CAL_DF0_WIDTH           12
#define RF_PLL_GAIN_CAL_DF1_LSB             12
#define RF_PLL_GAIN_CAL_DF1_WIDTH           12
#define RF_PLL_CAL_STATE_LSB                24
#define RF_PLL_CAL_STATE_WIDTH              5
//================================

typedef union //0x030 
{
    struct
    {
        uint32_t PLL_DAC_ADJ10:                      6; // bit[5:0]   ---
                                                        // 2408 ~ 2415
        uint32_t PLL_DAC_ADJ11:                      6; // bit[11:6]  ---
                                                        // 2424 ~ 2431
        uint32_t PLL_DAC_ADJ12:                      6; // bit[17:12] ---
                                                        // 2440 ~ 2447
        uint32_t PLL_DAC_ADJ13:                      6; // bit[23:18] ---
                                                        // 2456 ~ 2463
        uint32_t PLL_DAC_ADJ14:                      6; // bit[29:24] ---
                                                        // 2472 ~ 2479
        uint32_t RSV_END:                            2; // bit[31:30]
    };
    uint32_t Word;
} RF_PLL_DAC_TAB1_TypeDef; //0x030 


//================================
#define RF_PLL_DAC_ADJ10_LSB                0
#define RF_PLL_DAC_ADJ10_WIDTH              6
#define RF_PLL_DAC_ADJ11_LSB                6
#define RF_PLL_DAC_ADJ11_WIDTH              6
#define RF_PLL_DAC_ADJ12_LSB                12
#define RF_PLL_DAC_ADJ12_WIDTH              6
#define RF_PLL_DAC_ADJ13_LSB                18
#define RF_PLL_DAC_ADJ13_WIDTH              6
#define RF_PLL_DAC_ADJ14_LSB                24
#define RF_PLL_DAC_ADJ14_WIDTH              6
//================================

typedef union //0x034 
{
    struct
    {
        uint32_t PLL_DAC_ADJ05:                      6; // bit[5:0] ---
                                                        // 2480 ~ 2487
        uint32_t PLL_DAC_ADJ15:                      6; // bit[11:6] ---
                                                        // 2488 ~ 2495
        uint32_t RSV_END:                           20; // bit[31:12]
    };
    uint32_t Word;
} RF_PLL_DAC_TAB2_TypeDef; //0x034 


//================================
#define RF_PLL_DAC_ADJ05_LSB                0
#define RF_PLL_DAC_ADJ05_WIDTH              6
#define RF_PLL_DAC_ADJ15_LSB                6
#define RF_PLL_DAC_ADJ15_WIDTH              6
//================================

typedef union //0x038 
{
    struct
    {
        uint32_t SDM_DAT_SEL:                        1; // bit0 ---
                                                        // 0: SDM data from rf_ana
                                                        // 1: SDM data from mdm
        uint32_t MDM_SDM_DATA_DLY_1M:                3; // bit[3:1]
        uint32_t DAC_DATA_DLY_1M:                    3; // bit[6:4]
        uint32_t MDM_SDM_DATA_DLY_2M:                3; // bit[9:7]
        uint32_t DAC_DATA_DLY_2M:                    3; // bit[12:10]
        uint32_t SDM_CLK_PH:                         1; // bit13 ---
                                                        // 1: inverter the BAKIN_PFD clock for SDM
        uint32_t RSV_END:                           18; // bit[31:14]
    };
    uint32_t Word;
} RF_DATA_DLY_CTRL_TypeDef; //0x038 


//================================
#define RF_SDM_DAT_SEL_POS                  0
#define RF_MDM_SDM_DATA_DLY_1M_LSB          1
#define RF_MDM_SDM_DATA_DLY_1M_WIDTH        3
#define RF_DAC_DATA_DLY_1M_LSB              4
#define RF_DAC_DATA_DLY_1M_WIDTH            3
#define RF_MDM_SDM_DATA_DLY_2M_LSB          7
#define RF_MDM_SDM_DATA_DLY_2M_WIDTH        3
#define RF_DAC_DATA_DLY_2M_LSB              10
#define RF_DAC_DATA_DLY_2M_WIDTH            3
#define RF_SDM_CLK_PH_POS                   13
//================================

typedef union //0x03c 
{
    struct
    {
        uint32_t TX_DI_S_CHIDX37:                    5; // bit[4:0]
        uint32_t TX_FRAC_CHIDX37:                    4; // bit[8:5] ---
                                                        // high 4bits of the tx_frac[23:0], low 20bits is 0
        uint32_t RSV_NOUSE1:                         1; // bit9
        uint32_t TX_DI_S_CHIDX38:                    5; // bit[14:10]
        uint32_t TX_FRAC_CHIDX38:                    4; // bit[18:15] ---
                                                        // high 4bits of the tx_frac[23:0], low 20bits is 0
        uint32_t RSV_NOUSE2:                         1; // bit19
        uint32_t TX_DI_S_CHIDX39:                    5; // bit[24:20]
        uint32_t TX_FRAC_CHIDX39:                    4; // bit[28:25] ---
                                                        // high 4bits of the tx_frac[23:0], low 20bits is 0
        uint32_t RSV_END:                            3; // bit[31:29]
    };
    uint32_t Word;
} RF_TX_CH_MAP_TypeDef; //0x03c 


//================================
#define RF_TX_DI_S_CHIDX37_LSB              0
#define RF_TX_DI_S_CHIDX37_WIDTH            5
#define RF_TX_FRAC_CHIDX37_LSB              5
#define RF_TX_FRAC_CHIDX37_WIDTH            4
#define RF_TX_DI_S_CHIDX38_LSB              10
#define RF_TX_DI_S_CHIDX38_WIDTH            5
#define RF_TX_FRAC_CHIDX38_LSB              15
#define RF_TX_FRAC_CHIDX38_WIDTH            4
#define RF_TX_DI_S_CHIDX39_LSB              20
#define RF_TX_DI_S_CHIDX39_WIDTH            5
#define RF_TX_FRAC_CHIDX39_LSB              25
#define RF_TX_FRAC_CHIDX39_WIDTH            4
//================================

typedef union //0x040 
{
    struct
    {
        uint32_t RX_DI_S_CHIDX37:                    5; // bit[4:0]
        uint32_t RX_FRAC_CHIDX37:                   24; // bit[28:5]
        uint32_t RSV_END:                            3; // bit[31:29]
    };
    uint32_t Word;
} RF_RX_CHIDX37_MAP_TypeDef; //0x040 


//================================
#define RF_RX_DI_S_CHIDX37_LSB              0
#define RF_RX_DI_S_CHIDX37_WIDTH            5
#define RF_RX_FRAC_CHIDX37_LSB              5
#define RF_RX_FRAC_CHIDX37_WIDTH            24
//================================

typedef union //0x044 
{
    struct
    {
        uint32_t RX_DI_S_CHIDX38:                    5; // bit[4:0]
        uint32_t RX_FRAC_CHIDX38:                   24; // bit[28:5]
        uint32_t RSV_END:                            3; // bit[31:29]
    };
    uint32_t Word;
} RF_RX_CHIDX38_MAP_TypeDef; //0x044 


//================================
#define RF_RX_DI_S_CHIDX38_LSB              0
#define RF_RX_DI_S_CHIDX38_WIDTH            5
#define RF_RX_FRAC_CHIDX38_LSB              5
#define RF_RX_FRAC_CHIDX38_WIDTH            24
//================================

typedef union //0x048 
{
    struct
    {
        uint32_t RX_DI_S_CHIDX39:                    5; // bit[4:0]
        uint32_t RX_FRAC_CHIDX39:                   24; // bit[28:5]
        uint32_t RSV_END:                            3; // bit[31:29]
    };
    uint32_t Word;
} RF_RX_CHIDX39_MAP_TypeDef; //0x048 


//================================
#define RF_RX_DI_S_CHIDX39_LSB              0
#define RF_RX_DI_S_CHIDX39_WIDTH            5
#define RF_RX_FRAC_CHIDX39_LSB              5
#define RF_RX_FRAC_CHIDX39_WIDTH            24
//================================

typedef union //0x04c 
{
    struct
    {
        uint32_t BPF_BW_ADJ:                         2; // bit[1:0]
        uint32_t BPF_CAL_CODE_EXT:                   6; // bit[7:2]
        uint32_t BPF_CAL_CODE_EXT_EN:                1; // bit8
        uint32_t BPF_CAL_EN:                         1; // bit9 ---
                                                        // only used when pll_bpf_cal_by is 1
        uint32_t BPF_CENT_ADJ:                       2; // bit[11:10]
        uint32_t BPF_IADJ:                           3; // bit[14:12]
        uint32_t BPF_MODE_SEL:                       1; // bit15
        uint32_t MIXH_BIAS_CTL:                      3; // bit[18:16]
        uint32_t MIXH_BIAS_SEL:                      1; // bit19
        uint32_t MIXH_ENB_CAP:                       1; // bit20
        uint32_t MIXL_BIAS_CTL:                      3; // bit[23:21]
        uint32_t MIXL_BIAS_SEL:                      1; // bit24
        uint32_t PA_CAP:                             4; // bit[28:25]
        uint32_t RSV_END:                            3; // bit[31:29]
    };
    uint32_t Word;
} RF_BPFMIX_CTRL_TypeDef; //0x04c 


//================================
#define RF_BPF_BW_ADJ_LSB                   0
#define RF_BPF_BW_ADJ_WIDTH                 2
#define RF_BPF_CAL_CODE_EXT_LSB             2
#define RF_BPF_CAL_CODE_EXT_WIDTH           6
#define RF_BPF_CAL_CODE_EXT_EN_POS          8
#define RF_BPF_CAL_EN_POS                   9
#define RF_BPF_CENT_ADJ_LSB                 10
#define RF_BPF_CENT_ADJ_WIDTH               2
#define RF_BPF_IADJ_LSB                     12
#define RF_BPF_IADJ_WIDTH                   3
#define RF_BPF_MODE_SEL_POS                 15
#define RF_MIXH_BIAS_CTL_LSB                16
#define RF_MIXH_BIAS_CTL_WIDTH              3
#define RF_MIXH_BIAS_SEL_POS                19
#define RF_MIXH_ENB_CAP_POS                 20
#define RF_MIXL_BIAS_CTL_LSB                21
#define RF_MIXL_BIAS_CTL_WIDTH              3
#define RF_MIXL_BIAS_SEL_POS                24
#define RF_PA_CAP_LSB                       25
#define RF_PA_CAP_WIDTH                     4
//================================

typedef union //0x050 
{
    struct
    {
        uint32_t AGC_S00H:                           6; // bit[5:0]
        uint32_t AGC_S00L:                           6; // bit[11:6]
        uint32_t AGC_S00_BPF_ADJ:                    2; // bit[13:12]
        uint32_t AGC_S00_LNA_ADJ:                    2; // bit[15:14]
        uint32_t AGC_S00_LNA_BYPS_ADJ:               1; // bit16
        uint32_t AGC_S00_LNA_EN_ADJ:                 1; // bit17
        uint32_t AGC_S00_MIX_ADJ:                    2; // bit[19:18]
        uint32_t AGC_POWER_DET_EN:                   1; // bit20
        uint32_t AGC_TEST_EN:                        1; // bit21
        uint32_t AGC_TEST_S:                         2; // bit[23:22]
        uint32_t AGC_T_ADJ:                          2; // bit[25:24]
        uint32_t AGC_VH_ADD_ADJ:                     2; // bit[27:26]
        uint32_t DISABLE_AGC:                        1; // bit28
        uint32_t RSV_END:                            3; // bit[31:29]
    };
    uint32_t Word;
} RF_AGC_CTRL0_TypeDef; //0x050 


//================================
#define RF_AGC_S00H_LSB                     0
#define RF_AGC_S00H_WIDTH                   6
#define RF_AGC_S00L_LSB                     6
#define RF_AGC_S00L_WIDTH                   6
#define RF_AGC_S00_BPF_ADJ_LSB              12
#define RF_AGC_S00_BPF_ADJ_WIDTH            2
#define RF_AGC_S00_LNA_ADJ_LSB              14
#define RF_AGC_S00_LNA_ADJ_WIDTH            2
#define RF_AGC_S00_LNA_BYPS_ADJ_POS         16
#define RF_AGC_S00_LNA_EN_ADJ_POS           17
#define RF_AGC_S00_MIX_ADJ_LSB              18
#define RF_AGC_S00_MIX_ADJ_WIDTH            2
#define RF_AGC_POWER_DET_EN_POS             20
#define RF_AGC_TEST_EN_POS                  21
#define RF_AGC_TEST_S_LSB                   22
#define RF_AGC_TEST_S_WIDTH                 2
#define RF_AGC_T_ADJ_LSB                    24
#define RF_AGC_T_ADJ_WIDTH                  2
#define RF_AGC_VH_ADD_ADJ_LSB               26
#define RF_AGC_VH_ADD_ADJ_WIDTH             2
#define RF_DISABLE_AGC_POS                  28
//================================

typedef union //0x054 
{
    struct
    {
        uint32_t AGC_S01H:                           6; // bit[5:0]
        uint32_t AGC_S01L:                           6; // bit[11:6]
        uint32_t AGC_S01_BPF_ADJ:                    2; // bit[13:12]
        uint32_t AGC_S01_LNA_ADJ:                    2; // bit[15:14]
        uint32_t AGC_S01_LNA_BYPS_ADJ:               1; // bit16
        uint32_t AGC_S01_LNA_EN_ADJ:                 1; // bit17
        uint32_t AGC_S01_MIX_ADJ:                    2; // bit[19:18]
        uint32_t RSV_END:                           12; // bit[31:20]
    };
    uint32_t Word;
} RF_AGC_CTRL1_TypeDef; //0x054 


//================================
#define RF_AGC_S01H_LSB                     0
#define RF_AGC_S01H_WIDTH                   6
#define RF_AGC_S01L_LSB                     6
#define RF_AGC_S01L_WIDTH                   6
#define RF_AGC_S01_BPF_ADJ_LSB              12
#define RF_AGC_S01_BPF_ADJ_WIDTH            2
#define RF_AGC_S01_LNA_ADJ_LSB              14
#define RF_AGC_S01_LNA_ADJ_WIDTH            2
#define RF_AGC_S01_LNA_BYPS_ADJ_POS         16
#define RF_AGC_S01_LNA_EN_ADJ_POS           17
#define RF_AGC_S01_MIX_ADJ_LSB              18
#define RF_AGC_S01_MIX_ADJ_WIDTH            2
//================================

typedef union //0x058 
{
    struct
    {
        uint32_t AGC_S10L:                           6; // bit[5:0]
        uint32_t AGC_S10_BPF_ADJ:                    2; // bit[7:6]
        uint32_t AGC_S10_LNA_ADJ:                    2; // bit[9:8]
        uint32_t AGC_S10_LNA_BYPS_ADJ:               1; // bit10
        uint32_t AGC_S10_LNA_EN_ADJ:                 1; // bit11
        uint32_t AGC_S10_MIX_ADJ:                    2; // bit[13:12]
        uint32_t RSV_NOUSE1:                         2; // bit[15:14]
        uint32_t AGC_S11_BPF_ADJ:                    2; // bit[17:16]
        uint32_t AGC_S11_LNA_ADJ:                    2; // bit[19:18]
        uint32_t AGC_S11_LNA_BYPS_ADJ:               1; // bit20
        uint32_t AGC_S11_LNA_EN_ADJ:                 1; // bit21
        uint32_t AGC_S11_MIX_ADJ:                    2; // bit[23:22]
        uint32_t RSV_END:                            8; // bit[31:24]
    };
    uint32_t Word;
} RF_AGC_CTRL2_TypeDef; //0x058 


//================================
#define RF_AGC_S10L_LSB                     0
#define RF_AGC_S10L_WIDTH                   6
#define RF_AGC_S10_BPF_ADJ_LSB              6
#define RF_AGC_S10_BPF_ADJ_WIDTH            2
#define RF_AGC_S10_LNA_ADJ_LSB              8
#define RF_AGC_S10_LNA_ADJ_WIDTH            2
#define RF_AGC_S10_LNA_BYPS_ADJ_POS         10
#define RF_AGC_S10_LNA_EN_ADJ_POS           11
#define RF_AGC_S10_MIX_ADJ_LSB              12
#define RF_AGC_S10_MIX_ADJ_WIDTH            2
#define RF_AGC_S11_BPF_ADJ_LSB              16
#define RF_AGC_S11_BPF_ADJ_WIDTH            2
#define RF_AGC_S11_LNA_ADJ_LSB              18
#define RF_AGC_S11_LNA_ADJ_WIDTH            2
#define RF_AGC_S11_LNA_BYPS_ADJ_POS         20
#define RF_AGC_S11_LNA_EN_ADJ_POS           21
#define RF_AGC_S11_MIX_ADJ_LSB              22
#define RF_AGC_S11_MIX_ADJ_WIDTH            2
//================================

typedef union //0x05c 
{
    struct
    {
        uint32_t BG_BIAS_TRIM:                       2; // bit[1:0]
        uint32_t BG_RES_TRIM:                        5; // bit[6:2]
        uint32_t BG_VREF_FINE:                       3; // bit[9:7]
        uint32_t LDO_RX_TRIM:                        3; // bit[12:10]
        uint32_t LDO_TX_TRIM:                        3; // bit[15:13]
        uint32_t LNA_RES_TRIM:                       3; // bit[18:16]
        uint32_t BPF_GAIN_ADJ:                       2; // bit[20:19]
        uint32_t MIXL_GAIN_CTL:                      1; // bit21
        uint32_t MIXH_GAIN_CTL:                      1; // bit22
        uint32_t LNA_GAIN:                           4; // bit[26:23]
        uint32_t DAC_BLE_DELAY_ADJ_2M:               5; // bit[31:27]
    };
    uint32_t Word;
} RF_ANA_TRIM_TypeDef; //0x05c 


//================================
#define RF_BG_BIAS_TRIM_LSB                 0
#define RF_BG_BIAS_TRIM_WIDTH               2
#define RF_BG_RES_TRIM_LSB                  2
#define RF_BG_RES_TRIM_WIDTH                5
#define RF_BG_VREF_FINE_LSB                 7
#define RF_BG_VREF_FINE_WIDTH               3
#define RF_LDO_RX_TRIM_LSB                  10
#define RF_LDO_RX_TRIM_WIDTH                3
#define RF_LDO_TX_TRIM_LSB                  13
#define RF_LDO_TX_TRIM_WIDTH                3
#define RF_LNA_RES_TRIM_LSB                 16
#define RF_LNA_RES_TRIM_WIDTH               3
#define RF_BPF_GAIN_ADJ_LSB                 19
#define RF_BPF_GAIN_ADJ_WIDTH               2
#define RF_MIXL_GAIN_CTL_POS                21
#define RF_MIXH_GAIN_CTL_POS                22
#define RF_LNA_GAIN_LSB                     23
#define RF_LNA_GAIN_WIDTH                   4
#define RF_DAC_BLE_DELAY_ADJ_2M_LSB         27
#define RF_DAC_BLE_DELAY_ADJ_2M_WIDTH       5
//================================

typedef union //0x060 
{
    struct
    {
        uint32_t DAC_BLE_DELAY_ADJ_1M:               5; // bit[4:0]
        uint32_t DAC_REFL_ADJ:                       3; // bit[7:5]
        uint32_t DAC_CAL_DATA_EXT:                   1; // bit8
        uint32_t DAC_CAL_EN_EXT:                     1; // bit9
        uint32_t DAC_EXT_EN:                         1; // bit10
        uint32_t DAC_REFH_ADDJ:                      1; // bit11
        uint32_t DAC_REFH_ADJ:                       1; // bit12
        uint32_t BYP_LDOIF:                          1; // bit13
        uint32_t BYP_LDOPLL:                         1; // bit14
        uint32_t BYP_LDOTX:                          1; // bit15
        uint32_t BYP_LDOVCO:                         1; // bit16
        uint32_t CF_BW08M_ADJ:                       1; // bit17
        uint32_t RSV_NOUSE1:                         1; // bit18
        uint32_t TSTEN_CBPF:                         1; // bit19
        uint32_t TSTEN_RSSI:                         1; // bit20
        uint32_t AT0_SEL:                            4; // bit[24:21]
        uint32_t AT1_SEL:                            4; // bit[28:25]
        uint32_t VCO_RES:                            2; // bit[30:29]
        uint32_t VCOAFC_SEL:                         1; // bit31
    };
    uint32_t Word;
} RF_ANAMISC_CTRL1_TypeDef; //0x060 


//================================
#define RF_DAC_BLE_DELAY_ADJ_1M_LSB         0
#define RF_DAC_BLE_DELAY_ADJ_1M_WIDTH       5
#define RF_DAC_REFL_ADJ_LSB                 5
#define RF_DAC_REFL_ADJ_WIDTH               3
#define RF_DAC_CAL_DATA_EXT_POS             8
#define RF_DAC_CAL_EN_EXT_POS               9
#define RF_DAC_EXT_EN_POS                   10
#define RF_DAC_REFH_ADDJ_POS                11
#define RF_DAC_REFH_ADJ_POS                 12
#define RF_BYP_LDOIF_POS                    13
#define RF_BYP_LDOPLL_POS                   14
#define RF_BYP_LDOTX_POS                    15
#define RF_BYP_LDOVCO_POS                   16
#define RF_CF_BW08M_ADJ_POS                 17
#define RF_TSTEN_CBPF_POS                   19
#define RF_TSTEN_RSSI_POS                   20
#define RF_AT0_SEL_LSB                      21
#define RF_AT0_SEL_WIDTH                    4
#define RF_AT1_SEL_LSB                      25
#define RF_AT1_SEL_WIDTH                    4
#define RF_VCO_RES_LSB                      29
#define RF_VCO_RES_WIDTH                    2
#define RF_VCOAFC_SEL_POS                   31
//================================

typedef union //0x064 
{
    struct
    {
        uint32_t TEST_EN_LDO_VCO:                    1; // bit0 ---
                                                        // used when ldo_test_en is 1
        uint32_t TEST_EN_LDO_PA:                     1; // bit1 ---
                                                        // used when ldo_test_en is 1
        uint32_t TEST_EN_LDO_IF:                     1; // bit2 ---
                                                        // used when ldo_test_en is 1
        uint32_t TEST_EN_LDO_PLL:                    1; // bit3 ---
                                                        // used when ldo_test_en is 1
        uint32_t TEST_EN_DAC_DIG_PWR:                1; // bit4 ---
                                                        // used when ldo_test_en is 1
        uint32_t TEST_EN_AGC_PWR:                    1; // bit5 ---
                                                        // used when ldo_test_en is 1
        uint32_t TEST_EN_LDO_TX:                     1; // bit6 ---
                                                        // used when ldo_test_en is 1
        uint32_t RSV_NOUSE1:                         1; // bit7 ---
        uint32_t TX_EN_LDO_VCO:                      1; // bit8 ---
                                                        // used when RF is controlled by RF Dig FSM & in TX mode
        uint32_t TX_EN_LDO_PA:                       1; // bit9 ---
                                                        // used when RF is controlled by RF Dig FSM & in TX mode
        uint32_t TX_EN_LDO_IF:                       1; // bit10 ---
                                                        // used when RF is controlled by RF Dig FSM & in TX mode
        uint32_t TX_EN_LDO_PLL:                      1; // bit11 ---
                                                        // used when RF is controlled by RF Dig FSM & in TX mode
        uint32_t TX_EN_DAC_DIG_PWR:                  1; // bit12 ---
                                                        // used when RF is controlled by RF Dig FSM & in TX mode
        uint32_t TX_EN_AGC_PWR:                      1; // bit13 ---
                                                        // used when RF is controlled by RF Dig FSM & in TX mode
        uint32_t TX_EN_LDO_TX:                       1; // bit14 ---
                                                        // used when RF is controlled by RF Dig FSM & in TX mode
        uint32_t RSV_NOUSE2:                         1; // bit15 ---
        uint32_t RX_EN_LDO_VCO:                      1; // bit16 ---
                                                        // used when RF is controlled by RF Dig FSM & in RX mode
        uint32_t RX_EN_LDO_PA:                       1; // bit17 ---
                                                        // used when RF is controlled by RF Dig FSM & in RX mode
        uint32_t RX_EN_LDO_IF:                       1; // bit18 ---
                                                        // used when RF is controlled by RF Dig FSM & in RX mode
        uint32_t RX_EN_LDO_PLL:                      1; // bit19 ---
                                                        // used when RF is controlled by RF Dig FSM & in RX mode
        uint32_t RX_EN_DAC_DIG_PWR:                  1; // bit20 ---
                                                        // used when RF is controlled by RF Dig FSM & in RX mode
        uint32_t RX_EN_AGC_PWR:                      1; // bit21 ---
                                                        // used when RF is controlled by RF Dig FSM & in RX mode
        uint32_t RX_EN_LDO_TX:                       1; // bit22 ---
                                                        // used when RF is controlled by RF Dig FSM & in RX mode
        uint32_t CLK_EN_DAC:                         1; // bit23
        uint32_t CLK_EN_BPF:                         1; // bit24
        uint32_t CLK_EN_PLL:                         1; // bit25
        uint32_t CLK_EN_AGC:                         1; // bit26
        uint32_t RSV_END:                            5; // bit[31:27]
    };
    uint32_t Word;
} RF_ANA_PWR_CTRL_TypeDef; //0x064 


//================================
#define RF_TEST_EN_LDO_VCO_POS              0
#define RF_TEST_EN_LDO_PA_POS               1
#define RF_TEST_EN_LDO_IF_POS               2
#define RF_TEST_EN_LDO_PLL_POS              3
#define RF_TEST_EN_DAC_DIG_PWR_POS          4
#define RF_TEST_EN_AGC_PWR_POS              5
#define RF_TEST_EN_LDO_TX_POS               6
#define RF_TX_EN_LDO_VCO_POS                8
#define RF_TX_EN_LDO_PA_POS                 9
#define RF_TX_EN_LDO_IF_POS                 10
#define RF_TX_EN_LDO_PLL_POS                11
#define RF_TX_EN_DAC_DIG_PWR_POS            12
#define RF_TX_EN_AGC_PWR_POS                13
#define RF_TX_EN_LDO_TX_POS                 14
#define RF_RX_EN_LDO_VCO_POS                16
#define RF_RX_EN_LDO_PA_POS                 17
#define RF_RX_EN_LDO_IF_POS                 18
#define RF_RX_EN_LDO_PLL_POS                19
#define RF_RX_EN_DAC_DIG_PWR_POS            20
#define RF_RX_EN_AGC_PWR_POS                21
#define RF_RX_EN_LDO_TX_POS                 22
#define RF_CLK_EN_DAC_POS                   23
#define RF_CLK_EN_BPF_POS                   24
#define RF_CLK_EN_PLL_POS                   25
#define RF_CLK_EN_AGC_POS                   26
//================================

typedef union //0x068 
{
    struct
    {
        uint32_t EN_AGC_REG:                         1; // bit0 ---
                                                        // 0: signal is controlled by RF Dig FSM
                                                        // 1: the signal to analog is 1
        uint32_t EN_BPF_REG:                         1; // bit1 ---
                                                        // 0: signal is controlled by RF Dig FSM
                                                        // 1: the signal to analog is 1
        uint32_t EN_DAC_BLE_REG:                     1; // bit2 ---
                                                        // 0: signal is controlled by RF Dig FSM
                                                        // 1: the signal to analog is 1
        uint32_t EN_LMT_RSSI_REG:                    1; // bit3 ---
                                                        // 0: signal is controlled by RF Dig FSM
                                                        // 1: the signal to analog is 1
        uint32_t EN_LNA_REG:                         1; // bit4 ---
                                                        // 0: signal is controlled by RF Dig FSM
                                                        // 1: the signal to analog is 1
        uint32_t EN_MIXH_REG:                        1; // bit5 ---
                                                        // 0: signal is controlled by RF Dig FSM
                                                        // 1: the signal to analog is 1
        uint32_t EN_MIXL_REG:                        1; // bit6 ---
                                                        // 0: signal is controlled by RF Dig FSM
                                                        // 1: the signal to analog is 1
        uint32_t EN_PA_REG:                          1; // bit7 ---
                                                        // 0: signal is controlled by RF Dig FSM
                                                        // 1: the signal to analog is 1
        uint32_t EN_PLL_REG:                         1; // bit8 ---
                                                        // 0: signal is controlled by RF Dig FSM
                                                        // 1: the signal to analog is 1
        uint32_t EN_RSSI_I_REG:                      1; // bit9 ---
                                                        // 0: signal is controlled by RF Dig FSM
                                                        // 1: the signal to analog is 1
        uint32_t EN_RSSI_Q_REG:                      1; // bit10 ---
                                                        // 0: signal is controlled by RF Dig FSM
                                                        // 1: the signal to analog is 1
        uint32_t EN_FB_DIV_REG:                      1; // bit11 ---
                                                        // used when pll_cal_test_en as 1
        uint32_t PLL_CALIB_PD_REG:                   1; // bit12 ---
                                                        // used when pll_cal_test_en as 1
        uint32_t TX_OPEN_PD_REG:                     1; // bit13 ---
                                                        // used when pll_cal_test_en as 1
        uint32_t BAND_CAL_DONE_REG:                  1; // bit14 ---
                                                        // used when pll_cal_test_en as 1
        uint32_t GAIN_CAL_DONE_REG:                  1; // bit15 ---
                                                        // used when pll_cal_test_en as 1
        uint32_t EN_BG:                              1; // bit16 ---
                                                        // direct control RF ANA       
        uint32_t EN_LMT_OUTI_EXT:                    1; // bit17 ---
                                                        // direct control RF ANA       
        uint32_t EN_LMT_OUTQ_EXT:                    1; // bit18 ---
                                                        // direct control RF ANA       
        uint32_t EN_LNA_BYPS:                        1; // bit19 ---
                                                        // direct control RF ANA
        uint32_t EN_DAC_ZB:                          1; // bit20 ---
                                                        // direct control RF ANA
        uint32_t CF_BW12M_ADJ_REG:                   1; // bit21 ---
                                                        // used when ana_test_en is 1
        uint32_t DAC_VCTRL_BUF_REG:                  1; // bit22
        uint32_t DAC_VCTRL_EN_REG:                   1; // bit23
        uint32_t ANT_CAP_RX:                         4; // bit[27:24]
        uint32_t ANT_CAP_TX:                         4; // bit[31:28]
    };
    uint32_t Word;
} RF_ANA_EN_CTRL_TypeDef; //0x068 


//================================
#define RF_EN_AGC_REG_POS                   0
#define RF_EN_BPF_REG_POS                   1
#define RF_EN_DAC_BLE_REG_POS               2
#define RF_EN_LMT_RSSI_REG_POS              3
#define RF_EN_LNA_REG_POS                   4
#define RF_EN_MIXH_REG_POS                  5
#define RF_EN_MIXL_REG_POS                  6
#define RF_EN_PA_REG_POS                    7
#define RF_EN_PLL_REG_POS                   8
#define RF_EN_RSSI_I_REG_POS                9
#define RF_EN_RSSI_Q_REG_POS                10
#define RF_EN_FB_DIV_REG_POS                11
#define RF_PLL_CALIB_PD_REG_POS             12
#define RF_TX_OPEN_PD_REG_POS               13
#define RF_BAND_CAL_DONE_REG_POS            14
#define RF_GAIN_CAL_DONE_REG_POS            15
#define RF_EN_BG_POS                        16
#define RF_EN_LMT_OUTI_EXT_POS              17
#define RF_EN_LMT_OUTQ_EXT_POS              18
#define RF_EN_LNA_BYPS_POS                  19
#define RF_EN_DAC_ZB_POS                    20
#define RF_CF_BW12M_ADJ_REG_POS             21
#define RF_DAC_VCTRL_BUF_REG_POS            22
#define RF_DAC_VCTRL_EN_REG_POS             23
#define RF_ANT_CAP_RX_LSB                   24
#define RF_ANT_CAP_RX_WIDTH                 4
#define RF_ANT_CAP_TX_LSB                   28
#define RF_ANT_CAP_TX_WIDTH                 4
//================================

typedef union //0x06c 
{
    struct
    {
        uint32_t PLL_BW_ADJ:                         3; // bit[2:0]
        uint32_t PLL_CP_OS_EN:                       1; // bit3
        uint32_t PLL_CP_OS_ADJ:                      4; // bit[7:4]
        uint32_t PLL_DIV_ADJ:                        2; // bit[9:8]
        uint32_t PLL_DI_P:                           5; // bit[14:10]
        uint32_t PLL_FAST_LOCK_EN:                   1; // bit15
        uint32_t PLL_FBDIV_RST_EXT:                  1; // bit16
        uint32_t PLL_FBDIV_RST_SEL:                  1; // bit17
        uint32_t PLL_LOCK_BYPS:                      1; // bit18
        uint32_t PLL_PS_CNT_RST_SEL:                 1; // bit19
        uint32_t PLL_REF_SEL:                        1; // bit20
        uint32_t PLL_SDM_TEST_EN:                    1; // bit21
        uint32_t PLL_VCO_ADJ:                        3; // bit[24:22]
        uint32_t PLL_VCTRL_EXT_EN:                   1; // bit25
        uint32_t PLL_VREF_ADJ:                       3; // bit[28:26]
        uint32_t PLL_MIX_SEL:                        1; // bit29 ---
                                                        // 1: +2M IF for RX  
                                                        // 0: -2M IF for RX  
                                                        // to RF Analog block
        uint32_t PLL_RTX_SEL_REG:                    1; // bit30 ---
                                                        // used when ana_test_en is 1
        uint32_t PLL_SEL_RTX_BW:                     1; // bit31 ---
                                                        // direct to RF Analog block
    };
    uint32_t Word;
} RF_PLL_ANA_CTRL_TypeDef; //0x06c 


//================================
#define RF_PLL_BW_ADJ_LSB                   0
#define RF_PLL_BW_ADJ_WIDTH                 3
#define RF_PLL_CP_OS_EN_POS                 3
#define RF_PLL_CP_OS_ADJ_LSB                4
#define RF_PLL_CP_OS_ADJ_WIDTH              4
#define RF_PLL_DIV_ADJ_LSB                  8
#define RF_PLL_DIV_ADJ_WIDTH                2
#define RF_PLL_DI_P_LSB                     10
#define RF_PLL_DI_P_WIDTH                   5
#define RF_PLL_FAST_LOCK_EN_POS             15
#define RF_PLL_FBDIV_RST_EXT_POS            16
#define RF_PLL_FBDIV_RST_SEL_POS            17
#define RF_PLL_LOCK_BYPS_POS                18
#define RF_PLL_PS_CNT_RST_SEL_POS           19
#define RF_PLL_REF_SEL_POS                  20
#define RF_PLL_SDM_TEST_EN_POS              21
#define RF_PLL_VCO_ADJ_LSB                  22
#define RF_PLL_VCO_ADJ_WIDTH                3
#define RF_PLL_VCTRL_EXT_EN_POS             25
#define RF_PLL_VREF_ADJ_LSB                 26
#define RF_PLL_VREF_ADJ_WIDTH               3
#define RF_PLL_MIX_SEL_POS                  29
#define RF_PLL_RTX_SEL_REG_POS              30
#define RF_PLL_SEL_RTX_BW_POS               31
//================================

typedef union //0x074 
{
    struct
    {
        uint32_t AGC_STATE0:                         1; // bit0
        uint32_t AGC_STATE1:                         1; // bit1
        uint32_t BPF_CAL_CODE:                       6; // bit[7:2]
        uint32_t BG_VREF_OK12:                       1; // bit8
        uint32_t BPF_CAL_DONE:                       1; // bit9
        uint32_t PLL_LOCK:                           1; // bit10
        uint32_t FAST_LOCK_DONE:                     1; // bit11
        uint32_t AGC_STATE_TEST:                     2; // bit[13:12]
        uint32_t RSV_END:                           18; // bit[31:14]
    };
    uint32_t Word;
} RF_RF_ANA_ST0_TypeDef; //0x074 


//================================
#define RF_AGC_STATE0_POS                   0
#define RF_AGC_STATE1_POS                   1
#define RF_BPF_CAL_CODE_LSB                 2
#define RF_BPF_CAL_CODE_WIDTH               6
#define RF_BG_VREF_OK12_POS                 8
#define RF_BPF_CAL_DONE_POS                 9
#define RF_PLL_LOCK_POS                     10
#define RF_FAST_LOCK_DONE_POS               11
#define RF_AGC_STATE_TEST_LSB               12
#define RF_AGC_STATE_TEST_WIDTH             2
//================================

//================================
//BLOCK RF top struct define 
typedef struct
{
    __IO  RF_DIG_CTRL_TypeDef                    DIG_CTRL            ; // 0x000,  
    __IO  RF_PLL_TAB_OFFSET_TypeDef              PLL_TAB_OFFSET      ; // 0x004,  
    __IO  RF_PLL_GAIN_CTRL_TypeDef               PLL_GAIN_CTRL       ; // 0x008,  
    __IO  RF_RSSI_CTRL_TypeDef                   RSSI_CTRL           ; // 0x00c,  
    __IO  RF_PLL_DAC_TAB0_TypeDef                PLL_DAC_TAB0        ; // 0x010,  
    __I   RF_PLL_CAL_ST_TypeDef                  PLL_CAL_ST          ; // 0x014,  
    __IO  RF_PLL_FREQ_CTRL_TypeDef               PLL_FREQ_CTRL       ; // 0x018,  
    __IO  RF_PLL_DYM_CTRL_TypeDef                PLL_DYM_CTRL        ; // 0x01c,  
    __IO  RF_FSM_DLY_CTRL0_TypeDef               FSM_DLY_CTRL0       ; // 0x020,  
    __IO  RF_FSM_DLY_CTRL1_TypeDef               FSM_DLY_CTRL1       ; // 0x024,  
    __IO  uint32_t                               PLL_CAL_DAC_STEP    ; // 0x028, 
                                                                       // when pll_gain_cal_mode is 2'b10 :
                                                                       // pll_cal_dac_step = N * abs ( pll_gain_cal_df1 - pll_gain_cal_df0 ) / 8, 
                                                                       // N is software define , common value is 2 or 4
                                                                       // when pll_gain_cal_mode is 2'b11 :
                                                                       // pll_cal_dac_step = 4* abs ( pll_gain_cal_df1 - pll_gain_cal_df0 ) / 8
                                                                       // data format : ( 8,2) unsigned data 
    __I   RF_PLL_GAIN_CAL_VAL_TypeDef            PLL_GAIN_CAL_VAL    ; // 0x02c,  
    __IO  RF_PLL_DAC_TAB1_TypeDef                PLL_DAC_TAB1        ; // 0x030,  
    __IO  RF_PLL_DAC_TAB2_TypeDef                PLL_DAC_TAB2        ; // 0x034,  
    __IO  RF_DATA_DLY_CTRL_TypeDef               DATA_DLY_CTRL       ; // 0x038,  
    __IO  RF_TX_CH_MAP_TypeDef                   TX_CH_MAP           ; // 0x03c,  
    __IO  RF_RX_CHIDX37_MAP_TypeDef              RX_CHIDX37_MAP      ; // 0x040,  
    __IO  RF_RX_CHIDX38_MAP_TypeDef              RX_CHIDX38_MAP      ; // 0x044,  
    __IO  RF_RX_CHIDX39_MAP_TypeDef              RX_CHIDX39_MAP      ; // 0x048,  
    __IO  RF_BPFMIX_CTRL_TypeDef                 BPFMIX_CTRL         ; // 0x04c,  
    __IO  RF_AGC_CTRL0_TypeDef                   AGC_CTRL0           ; // 0x050,  
    __IO  RF_AGC_CTRL1_TypeDef                   AGC_CTRL1           ; // 0x054,  
    __IO  RF_AGC_CTRL2_TypeDef                   AGC_CTRL2           ; // 0x058,  
    __IO  RF_ANA_TRIM_TypeDef                    ANA_TRIM            ; // 0x05c,  
    __IO  RF_ANAMISC_CTRL1_TypeDef               ANAMISC_CTRL1       ; // 0x060,  
    __IO  RF_ANA_PWR_CTRL_TypeDef                ANA_PWR_CTRL        ; // 0x064,  
    __IO  RF_ANA_EN_CTRL_TypeDef                 ANA_EN_CTRL         ; // 0x068, 
    __IO  RF_PLL_ANA_CTRL_TypeDef                PLL_ANA_CTRL        ; // 0x06c,  
    __IO  uint32_t                               RF_RSV              ; // 0x070, 
                                                                       // [0] : RF TEMP    connect to ADC_VIN[15]
                                                                       // [1] : RF BANDGAP connect to ADC_VIN[15]
                                                                       // [2] : RF RSSI    connect to ADC_VIN[15]
                                                                       // [3] : RF VDD_IF  connect to ADC_VIN[15]
                                                                       // [15]: CLK_128M_DAC enable signal 
    __I   RF_RF_ANA_ST0_TypeDef                  RF_ANA_ST0          ; // 0x074,  
} RF_TypeDef;


#define RF  (( RF_TypeDef  *)     RF_BASE)

#endif
