#include "rfctrl.h"

#include "reg_rf.h"
#include "rom.h"
#include "rcc.h"

#define USE_DAC_TAB   (1)
// 校准6次,去掉最大最小值, 剩余4次求平均.
#define GAIN_CAL_CNT  (6) // 1max+1min+4avg
#define SW_PA_TARGET  (0x01)
#define RF_CAL_VALID  (0x5AA5)
// FT烧录的RF电流阈值判断.
#define RF_0dBm_LIMIT (1100)

// 采样9次获取出现次数最多的AFC Code.
#define AFC_SAMPLE_CNT (9)

struct rf_cal_env_tag
{
    uint32_t bpf_ctrl;
    uint32_t dac_tab0;
    uint32_t dac_tab1;
    uint32_t dac_tab2; // bit[11:0] - dac_tab2, bit[15:12] - pa_target, bit[31:16] - cal_valid
};

struct pa_lvl_tag
{
    uint16_t lvl_c_curr;
    uint16_t lvl_d_curr;
    uint16_t lvl_e_curr;
    uint16_t lvl_f_curr;
    uint32_t ft_ver;
};

struct rf_cal_env_tag rf_cal_env;

#define RF_CAL_IS_DONE() ((rf_cal_env.dac_tab2 >> 16) == RF_CAL_VALID)
#define RF_CAL_VAL_CLR() rf_cal_env.dac_tab2 = 0
#define RF_CAL_VAL_SET() rf_cal_env.dac_tab2 |= (RF_CAL_VALID << 16)
#define RF_PLL_DYM_GET() ((rf_cal_env.dac_tab2 >> 12) & 0xF)
#define RF_PLL_DYM_SET(pa)                                                                         \
    rf_cal_env.dac_tab2 = (rf_cal_env.dac_tab2 & ~(0xF << 12)) | (((uint32_t)(pa) & 0xF) << 12)

/**
 * @brief  获取PLL频率调整状态（众数）
 * @param  sys_clk: 系统时钟配置
 * @param  chan_num: 通道号 (0-39)
 * @return 出现频率最高的 PLL_FREQ_ADJ_ST 值
 * @note   调用 AFC_SAMPLE_CNT 次取众数，提高稳定性
 */
uint8_t get_pll_freq_adj_st(uint8_t is_rx_mode, uint8_t sys_clk, uint8_t chnl)
{
    uint8_t results[AFC_SAMPLE_CNT];
    uint8_t max_count = 0;
    uint8_t max_value = 0;

    // 调用AFC_SAMPLE_CNT次获取结果
    for (uint8_t i = 0; i < AFC_SAMPLE_CNT; i++)
    {
        RF->PLL_DYM_CTRL.Word = is_rx_mode ? ((chnl << RF_SW_CH_NUM_LSB) | (1 << RF_SW_RX_EN_POS))
                                           : ((chnl << RF_SW_CH_NUM_LSB) | (1 << RF_SW_TX_EN_POS) |
                                                 (SW_PA_TARGET << RF_SW_PA_GAIN_TARGET_LSB));
        // delay some us for sync, then wait done
        btmr_delay(sys_clk, 200);

        results[i] = RF->PLL_CAL_ST.PLL_FREQ_ADJ_ST;

        RF->PLL_DYM_CTRL.Word = 0; //(SW_PA_TARGET << RF_SW_PA_GAIN_TARGET_LSB);
        btmr_delay(sys_clk, 100);
    }

    // 统计每个值的出现次数，返回众数
    for (uint8_t i = 0; i < AFC_SAMPLE_CNT; i++)
    {
        uint8_t count = 0;
        for (uint8_t j = 0; j < AFC_SAMPLE_CNT; j++)
        {
            if (results[j] == results[i])
            {
                count++;
            }
        }
        if (count > max_count)
        {
            max_count = count;
            max_value = results[i];
        }
    }

    return max_value;
}

uint8_t tx_afc_code[BLE_CH_NB], rx_afc_code[BLE_CH_NB];

void get_afc_code(uint8_t sys_clk)
{
    uint32_t dig_ctrl_reg = RF->DIG_CTRL.Word;

    RF->PLL_DYM_CTRL.Word     = 0;
    RF->DIG_CTRL.FSM_CTRL_SEL = 1;
    RF->DIG_CTRL.PLL_FREQ_SEL = 0;

    for (uint8_t ch = 0; ch < BLE_CH_NB; ++ch)
    {
        rx_afc_code[ch] = get_pll_freq_adj_st(1, sys_clk, ch);
        tx_afc_code[ch] = get_pll_freq_adj_st(0, sys_clk, ch);
    }

    RF->PLL_DYM_CTRL.Word = 0;

    RF->DIG_CTRL.Word = dig_ctrl_reg;
}

static uint8_t rf_chnl_dac(uint8_t sys_clk, uint8_t chnl)
{
    uint8_t value;

    RF->PLL_DYM_CTRL.Word = ((chnl << RF_SW_CH_NUM_LSB) | (1 << RF_SW_TX_EN_POS) |
                             (SW_PA_TARGET << RF_SW_PA_GAIN_TARGET_LSB));

    // delay some us for sync, then wait done
    btmr_delay(sys_clk, 200);

    while (!(RF->PLL_CAL_ST.GAIN_CALIB_DONE));

    // Get calib value
    value = RF->PLL_CAL_ST.PLL_GAIN_CAL_ST;

    RF->PLL_DYM_CTRL.Word = 0; //(SW_PA_TARGET << RF_SW_PA_GAIN_TARGET_LSB);

    btmr_delay(sys_clk, 100);

    return (value);
}

static uint32_t rf_dac_adjust(uint8_t sys_clk, uint8_t chnl)
{
    uint32_t sum = 0;
    uint8_t  lo = 0xFF, hi = 0x00, val;

    RF->PLL_TAB_OFFSET.Word = (tx_afc_code[chnl] << RF_PLL_FREQ_EXT_LSB);
    // .PLL_AFC_ROUND 1-->0, .PLL_FREQ_EXT_EN = 1
    RF->DIG_CTRL.Word = 0x0032015A;

    for (uint8_t i = 0; i < GAIN_CAL_CNT; ++i)
    {
        val = rf_chnl_dac(sys_clk, chnl);

        if (val < lo)
        {
            lo = val;
        }
        if (val > hi)
        {
            hi = val;
        }

        sum += val;
    }

    return (sum - lo - hi) / (GAIN_CAL_CNT - 2);
}

static void rf_gain_cal(uint8_t sys_clk)
{
    uint32_t dac_tab0, dac_tab1, dac_tab2;

    /******** 低频段阈值 2400~2439MHz ********/
    // RF->PLL_GAIN_CTRL.PLL_GAIN_CAL_TH   = 230; //按照287.5KHz计算 --- 2023.6.24 lch.
    RF->PLL_GAIN_CTRL.Word = (0x00002600 | (222 << RF_PLL_GAIN_CAL_TH_LSB));

    // 2400~2407 : 2406, 2402
    dac_tab0 = rf_dac_adjust(sys_clk, 1);

    // 2408~2415 : 2410, 2412
    dac_tab1 = rf_dac_adjust(sys_clk, 3);

    // 2416~2423 : 2418, 2420
    dac_tab0 |= rf_dac_adjust(sys_clk, 9) << RF_PLL_DAC_ADJ01_LSB;

    // 2424~2431 : 2428, 2430
    dac_tab1 |= rf_dac_adjust(sys_clk, 11) << RF_PLL_DAC_ADJ11_LSB;

    // 2432~2439 : 2434, 2436
    dac_tab0 |= rf_dac_adjust(sys_clk, 14) << RF_PLL_DAC_ADJ02_LSB;

    /******** 高频段减小阈值 2440~2480MHz ********/
    // RF->PLL_GAIN_CTRL.PLL_GAIN_CAL_TH   = 220; //按照287.5KHz计算 --- 2023.6.24 lch.
    //    RF->PLL_GAIN_CTRL.Word = (0x00002600 | (220 << RF_PLL_GAIN_CAL_TH_LSB));

    // 2440~2447 : 2442, 2444
    dac_tab1 |= rf_dac_adjust(sys_clk, 18) << RF_PLL_DAC_ADJ12_LSB;

    // 2448~2455 : 2450, 2452
    dac_tab0 |= rf_dac_adjust(sys_clk, 22) << RF_PLL_DAC_ADJ03_LSB;

    // 2456~2463 : 2458, 2460
    dac_tab1 |= rf_dac_adjust(sys_clk, 26) << RF_PLL_DAC_ADJ13_LSB;

    // 2464~2471 : 2466, 2468
    dac_tab0 |= rf_dac_adjust(sys_clk, 30) << RF_PLL_DAC_ADJ04_LSB;

    // 2472~2479 : 2474, 2476
    dac_tab1 |= rf_dac_adjust(sys_clk, 34) << RF_PLL_DAC_ADJ14_LSB;

    // 2480~2487 : 2480
    dac_tab2 = rf_dac_adjust(sys_clk, 39);

    // 2488~2495 : same with 2480
    dac_tab2 |= dac_tab2 << RF_PLL_DAC_ADJ15_LSB;

    /******** Write DAC_ADJ to Registers for done *******/
    RF->PLL_DAC_TAB0.Word = dac_tab0;
    RF->PLL_DAC_TAB1.Word = dac_tab1;
    RF->PLL_DAC_TAB2.Word = dac_tab2;

    /******** Retention in Memroy for reuse-done ********/
    rf_cal_env.dac_tab0 = dac_tab0;
    rf_cal_env.dac_tab1 = dac_tab1;
    rf_cal_env.dac_tab2 = dac_tab2;
    RF_CAL_VAL_SET();
}

static void rf_cbpf_cal(uint8_t sys_clk)
{
    RF->BPFMIX_CTRL.Word = 0x00437C90;

    // .PLL_AFC_ROUND 1-->0
    RF->DIG_CTRL.Word = 0x0022121A;

    // Rx 2440M
    RF->PLL_DYM_CTRL.Word = ((17 << RF_SW_CH_NUM_LSB) | (1 << RF_SW_RX_EN_POS));

    // wait BPF_CAL_DONE is 1, then must delay some us for sync.
    while (RF->RF_ANA_ST0.BPF_CAL_DONE == 0);
    btmr_delay(sys_clk, 100);

    // Get calib value
    // cbpf_code = RF->RF_ANA_ST0.BPF_CAL_CODE; -- bit[7:2] to RF_BPF_CAL_CODE_EXT_LSB
    rf_cal_env.bpf_ctrl = 0x00437D00 | (RF->RF_ANA_ST0.Word & 0xFC) /* | (0UL << RF_PA_CAP_LSB)*/;

    RF->BPFMIX_CTRL.Word  = rf_cal_env.bpf_ctrl;
    RF->PLL_DYM_CTRL.Word = 0;

    btmr_delay(sys_clk, 20);

    // .PLL_AFC_ROUND 1-->0
    RF->DIG_CTRL.Word = 0x0022125A;
}

void rf_reset(void)
{
    // Retrieve trim value
    uint8_t bg_res_trim;
    uint8_t sw_pa_gain;
    uint8_t dbg_sel = RF->DIG_CTRL.RF_DBG_SEL;
    uint8_t sys_clk = (rcc_sysclk_get() + 1) << 4;

    bg_res_trim = RF->ANA_TRIM.BG_RES_TRIM;

    sw_pa_gain = RF->PLL_DYM_CTRL.SW_PA_GAIN_TARGET;

#if (RF_0dBm_LIMIT)
    struct pa_lvl_tag *p_pa_lvl = (struct pa_lvl_tag *)(0x18000F1C);
    uint32_t           ft_ver   = p_pa_lvl->ft_ver;

    // FT RF_VBG 1.15V [ft_ver: SOP16 0x10, QFN32: 0x43, other:0xFFFFFFFF]
    if ((!RF_CAL_IS_DONE()) && ((ft_ver == 0xFFFFFFFF) || (ft_ver == 0x10) || (ft_ver == 0x43)))
    {
        bg_res_trim += 4;
        if (bg_res_trim > 0x1F)
        {
            bg_res_trim = 0x1F;
        }
    }

    // 2025.01.21 --- FT Trim.
    if ((sw_pa_gain == 0x8) && (RF_PLL_DYM_GET() == 0))
    {
        sw_pa_gain = (p_pa_lvl->lvl_c_curr > RF_0dBm_LIMIT)   ? 0x0C
                     : (p_pa_lvl->lvl_d_curr > RF_0dBm_LIMIT) ? 0x0D
                     : (p_pa_lvl->lvl_e_curr > RF_0dBm_LIMIT) ? 0x0E
                                                              : 0x0F;
    }
#else
    if ((sw_pa_gain == 0x8) && (RF_PLL_DYM_GET() == 0))
    {
        sw_pa_gain = 0x0D;
    }
#endif
    // Reset all config and state
    RCC->APBRST_CTRL.RF_RSTREQ = 1;
    RCC->APBRST_CTRL.RF_RSTREQ = 0;

    // Config registers. AFC_START_TIME 3-->7
    RF->FSM_DLY_CTRL0.Word = 0x0045E711;
    //.PA_STEP_TIME = 3
    RF->FSM_DLY_CTRL1.Word = 0x03120628;

    if (sys_clk == 48)
    {
        RF->DATA_DLY_CTRL.Word = 0x0000038B; //.SDM_CLK_PH=0
    }
    else
    {
        RF->DATA_DLY_CTRL.Word = 0x0000238B;
    }

    RF->BPFMIX_CTRL.Word = 0x00437D90;
    RF->AGC_CTRL0.Word   = 0x050BD572;
    RF->AGC_CTRL1.Word   = 0x000966B2;
    RF->AGC_CTRL2.Word   = 0x00E0331A;
    //.BG_RES_TRIM = 0x14

    RF->ANA_TRIM.Word = (0x07DAFF01 | (uint32_t)bg_res_trim << RF_BG_RES_TRIM_LSB);
    // reg default:0x00000890, DAC_BLE_DELAY_ADJ_1M=0
    RF->ANAMISC_CTRL1.Word = 0x00000880;
    //.TEST_EN_LDO_PLL = 1
    RF->ANA_PWR_CTRL.Word = 0x07FF7F08;
    RF->ANA_EN_CTRL.Word  = 0x00070000;
    // reg default:0x3180FE00
    // RF->PLL_ANA_CTRL.Word  = 0x3180FE00;
    RF->RF_RSV = 0x0000B804;

    if (RF_CAL_IS_DONE())
    {
        // cbpf value
        RF->BPFMIX_CTRL.Word = rf_cal_env.bpf_ctrl;
        // gain table
        RF->PLL_DAC_TAB0.Word = rf_cal_env.dac_tab0;
        RF->PLL_DAC_TAB1.Word = rf_cal_env.dac_tab1;
        RF->PLL_DAC_TAB2.Word = rf_cal_env.dac_tab2 & 0x0FFF;
    }
    else
    {
        rf_cbpf_cal(sys_clk);
        get_afc_code(sys_clk);
        rf_gain_cal(sys_clk);
    }

    if (sw_pa_gain)
    {
        RF_PLL_DYM_SET(sw_pa_gain);
    }

    // RF->PLL_DYM_CTRL.Word = (rf_cal_env.pll_dym_ctrl > 0) ? rf_cal_env.pll_dym_ctrl :
    // ((pll_dym_ctrl == 0x2000) ? 0x3C00 : pll_dym_ctrl);
    RF->PLL_DYM_CTRL.Word = ((uint32_t)sw_pa_gain << RF_SW_PA_GAIN_TARGET_LSB);

    // .PA_STEP_TIME = 7
    //    RF->FSM_DLY_CTRL1.Word = 0x07120628;

    // .PLL_AFC_ROUND 1-->0, .PLL_FREQ_EXT_EN = 1
    RF->DIG_CTRL.Word =
        0x00221352 | (0x01UL << RF_FSM_CTRL_SEL_POS) | (dbg_sel << RF_RF_DBG_SEL_LSB);
}

void rf_ctrl_init(void)
{
    RF_CAL_VAL_CLR();

    /* 1. reg_rf Init */
    rf_reset();

    /* 2. reg_mdm Init */
    RCC->APBRST_CTRL.MDM_RSTREQ = 1;
    RCC->APBRST_CTRL.MDM_RSTREQ = 0;
    MDM->REG0.Word              = 0x00000081 | (0x01UL << MDM_ACC_REG_EN_POS);
    MDM->REG1.Word              = 0x03E834B0;
    MDM->EXT_CTRL.Word          = 0x0F;
}

void rf_pa_set(uint8_t pa_target)
{
    uint32_t pa_gain = (pa_target & 0x0F);

    RF->PLL_DYM_CTRL.Word = pa_gain << RF_SW_PA_GAIN_TARGET_LSB;
    RF_PLL_DYM_SET(pa_gain);
}

uint8_t rf_pa_get(void)
{
    return (rf_cal_env.dac_tab2 >> 12) & 0xF;
}

void rf_recal_reset(void)
{
    rf_cal_env.dac_tab2 &= ~(0xFFFFU << 16);

    rf_reset();
}
