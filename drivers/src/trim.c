/**
 ****************************************************************************************
 *
 * @file trim.c
 *
 * @brief Triming校准值函数，内部使用，不对外开放。
 *
 ****************************************************************************************
 */
 #include "b6x.h"
#include "regs.h"
#include "rcc.h"
#include "trim.h"

//first sector(4k Bytes) use to keep trim info
//#define FSH_TRIM_ADDR           0x18000FDC
#define FSH_TRIM_ADDR           0x18000F00
#define TRIM_VAL_NUM            7 // vdd12_vol + VAL00~VAL05

#define TRIM_VALID_IDX          6 // TRIM_VAL05
#define TRIM_VALID_VALUE        0xC8F5D5B6
#define TRIM_VALID_OFFSET       (TRIM_VALID_IDX * sizeof(uint32_t)) // 0x14
#define TRIM_TOTAL_SIZE         (TRIM_VAL_NUM * sizeof(uint32_t))   // 0x18

// TRIM 0A: Flash XorKey
typedef union
{
    struct
    {
        uint32_t XOSC16M_CAP_TRIM_VAILD:  4;  // bit[3:0]
        uint32_t XOSC16M_CAP_TRIM:        6;  // bit[9:4]
        uint32_t Reserved:               22;  // bit[31:10]
    };
    uint32_t Word;
} TRIM_VAL00_Typedef;

typedef union
{
    struct
    {
        uint32_t RANDOM_SEED;         // bit[31:0]
    };
    uint32_t Word;
} TRIM_VAL01_Typedef;

typedef union
{
    struct
    {
        uint32_t LDO_TX_TRIM:       3; // bit[2:0]
        uint32_t LDO_RX_TRIM:       3; // bit[5:3]
        uint32_t BPF_CAL_CODE_EXT:  6; // bit[11:6]
        uint32_t BG_RES_TRIM:       5; // bit[16:12]
        uint32_t BG_BIAS_TRIM:      2; // bit[18:17]
        uint32_t MIC_TRIM:          7; // bit[25:19]
        uint32_t Reserved:          6; // bit[31:26] add 6vp 1118
    };
    uint32_t Word;
} TRIM_VAL02_Typedef;

typedef union
{
    struct
    {
        uint32_t RC16M_FREQ_TRIM_VAILD:   4; // bit[3:0]
        uint32_t RC16M_FREQ_TRIM:         6; // bit[9:4]
        uint32_t RC16M_FREQ_VALUE:       12; // bit[21:10]  // rc16m freq value
                                             // [21:17] integer(8~24MHz), [16:10] decimal((0~99)*10KHz)
        uint32_t Reserved:                2; // bit[23:22]
        uint32_t LDO_XOSC_TR_VAILD:       4; // bit[27:24]
        uint32_t LDO_XOSC_TR:             4; // bit[31:28]
    };
    uint32_t Word;
} TRIM_VAL03_Typedef;

typedef union
{
    struct
    {
        uint32_t VOLTAGE_TRIM_VALID:    4;   // bit[3:0]
        uint32_t CORELDO_TRIM_RUN:      5;   // bit[8:4]
        uint32_t CORELDO_TRIM_DP:       5;   // bit[13:9]
        uint32_t AONLDO_TRIM_RUN:       4;   // bit[17:14]
        uint32_t AONLDO_TRIM_OFF:       4;   // bit[21:18]
        uint32_t Reserved:              4;   // bit[25:22]
        uint32_t BK_BOD_TRIM:           3;   // bit[28:26]
        uint32_t LDO_LVD_SEL:           3;   // bit[31:29]
    };
    uint32_t Word;
} TRIM_VAL04_Typedef;

typedef union
{
    struct
    {
        uint32_t TRIM_VALID_FLAG;         // bit[31:0]
    };
    uint32_t Word;
} TRIM_VAL05_Typedef;

typedef struct
{
    TRIM_VAL00_Typedef  VAL00;
    TRIM_VAL01_Typedef  VAL01;
    TRIM_VAL02_Typedef  VAL02;
    TRIM_VAL03_Typedef  VAL03;
    TRIM_VAL04_Typedef  VAL04;
    TRIM_VAL05_Typedef  VAL05;
} TRIM_Typedef;

void trim_load(void)
{
    // 10ms iwdt rst.
    // fixbug--Low temperature poweron run fly. 2024.06.27 --- 6vp.
    iwdt_conf(160);

    // adjust coreldo and aonldo voltage to 1.2v
    // AONLDO VOL > CORELDO VOL, BKHOLD_CTRL defult:0x2008
//    AON->BKHOLD_CTRL.CORELDO_TRIM_RUN      = 0x1B;
//    AON->BKHOLD_CTRL.AONLDO_TRIM_RUN       = 0x01;

    // LDO_UD_CTRL default:0x100
    // deepsleep mode, coreldo auto undertension to 1.0v
//    APBMISC->LDO_UD_CTRL.CORELDO_TRIM_DP   = 0x14;
//    APBMISC->LDO_UD_CTRL.CORELDO_TRIM_STEP = 0x03;

    // poweroff mode, aonldo auto undertension to 1.0v
//    APBMISC->LDO_UD_CTRL.AONLDO_TRIM_OFF   = 0x04;
//    APBMISC->LDO_UD_CTRL.AONLDO_UD_STEP    = 0x07;

    // DBG_CTRL default:0x00
//    SYSCFG->DBG_CTRL.IWDT_DEBUG            = 1;
    SYSCFG->DBG_CTRL.Word = (0x01UL << SYSCFG_IWDT_DEBUG_POS);

    // flash trim vaild judge
    if (RD_32(FSH_TRIM_ADDR + 0x18) != TRIM_VALID_VALUE)
    {
        AON->BKHOLD_CTRL.Word     = (0x01B1UL << AON_AONLDO_TRIM_RUN_LSB) | (AON->BKHOLD_CTRL.Word & 0xFFFFUL);
        APBMISC->LDO_UD_CTRL.Word = 0x00740314;
        return ; // trim invaild
    }

    uint32_t i, values[TRIM_VAL_NUM]; // 'i' reduce size, 6vp 1118

    // values[0]: VDD12 Voltage
    TRIM_Typedef *trim = (TRIM_Typedef *)(values + 1);

    for (i = 0; i < (TRIM_VAL_NUM - 1); i++)
    {
        values[i] = RD_32(FSH_TRIM_ADDR + (i << 2));
    }

    // val04
//    AON->PMU_ANA_CTRL.BK_BOD_TRIM = trim->VAL04.BK_BOD_TRIM;
//    AON->PMU_ANA_CTRL.LDO_LVD_SEL = trim->VAL04.LDO_LVD_SEL;

    if ((trim->VAL04.Word != 0xFFFFFFFF) && (trim->VAL04.Word & 0x0A) == 0x0A)
    {
        AON->BKHOLD_CTRL.Word     = (trim->VAL04.CORELDO_TRIM_RUN << AON_CORELDO_TRIM_RUN_LSB)
                                    | (trim->VAL04.AONLDO_TRIM_RUN << AON_AONLDO_TRIM_RUN_LSB)
                                    | (AON->BKHOLD_CTRL.Word & 0xFFFFUL);

        APBMISC->LDO_UD_CTRL.Word = (trim->VAL04.CORELDO_TRIM_DP << APBMISC_CORELDO_TRIM_DP_LSB)
                                    | (trim->VAL04.AONLDO_TRIM_OFF << APBMISC_AONLDO_TRIM_OFF_LSB)
                                    | (0x07 << APBMISC_AONLDO_UD_STEP_LSB) | (0x03 << APBMISC_CORELDO_TRIM_STEP_LSB);
        // coreldo
//        AON->BKHOLD_CTRL.CORELDO_TRIM_RUN    = trim->VAL04.CORELDO_TRIM_RUN;
//        APBMISC->LDO_UD_CTRL.CORELDO_TRIM_DP = trim->VAL04.CORELDO_TRIM_DP;

        // aonldo
//        AON->BKHOLD_CTRL.AONLDO_TRIM_RUN     = trim->VAL04.AONLDO_TRIM_RUN;
//        APBMISC->LDO_UD_CTRL.AONLDO_TRIM_OFF = trim->VAL04.AONLDO_TRIM_OFF;
    }

    if ((trim->VAL03.Word != 0xFFFFFFFF) && (trim->VAL03.Word & 0x0A) == 0x0A)
    {
        APBMISC->RC16M_FREQ_TRIM = trim->VAL03.RC16M_FREQ_TRIM;
    }

    if ((trim->VAL00.Word != 0xFFFFFFFF) && (trim->VAL00.Word & 0x0A) == 0x0A)
    {
        APBMISC->XOSC16M_CTRL.XOSC16M_CAP_TR = trim->VAL00.XOSC16M_CAP_TRIM;
    }

    if ((trim->VAL02.Word != 0xFFFFFFFF) && (trim->VAL02.Word != 0x00))
    {
        uint32_t reg_val = RCC->APBCLK_EN_RUN.Word;
        RCC_APBCLK_EN(APB_RF_BIT);
        RF->ANA_TRIM.BG_RES_TRIM = (trim->VAL02.Word & 0x0001F000) >> 12;
//        RCC_APBCLK_DIS(APB_RF_BIT);
        RCC->APBCLK_EN_RUN.Word = reg_val;
    }

    // no used
//    if ((trim->VAL03.Word & 0x0A000000) == 0x0A000000)
//    {
//        APBMISC->XOSC16M_CTRL.LDO_XOSC_TR = trim->VAL03.LDO_XOSC_TR;
//    }
    iwdt_conf(0x20000); //restore iwdt rst 8192ms.
}

uint32_t get_trim_rc16m_freq(void)
{
    uint32_t val = RD_32(FSH_TRIM_ADDR + 0x10); //0x001F35BA
    uint32_t rc16m_freq = 0;

    if ((val != 0xFFFFFFFF) && (val & 0x0A) == 0x0A)
    {
        rc16m_freq = (((val & 0x003E0000) >> 17) * 1000000) + (((val & 0x0001FC00) >> 10) * 10000);
    }

    return rc16m_freq;
}

uint32_t get_trim_vdd12_voltage(void)
{
    uint32_t val = RD_32(FSH_TRIM_ADDR + 0x00);

    return ((val == 0xFFFFFFFF) ? 1200 : (val - 4));
}

//uint32_t get_trim_rand_seed(void)
//{
//    return RD_32(FSH_TRIM_ADDR + 0x08);
//}
