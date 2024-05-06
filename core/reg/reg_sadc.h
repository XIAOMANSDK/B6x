#ifndef _REG_SADC_H_
#define _REG_SADC_H_

#include "reg_base.h"

//================================
//BLOCK SADC define

#define SADC_BASE                              ((uint32_t)0x40007000)
#define SADC_CTRL_ADDR_OFFSET                  0x000
#define SADC_AUTO_SW_CTRL_ADDR_OFFSET          0x004
#define SADC_CH_CTRL_ADDR_OFFSET               0x008
#define SADC_STCTRL_ADDR_OFFSET                0x00c
#define SADC_DC_OFFSET_ADDR_OFFSET             0x010
#define SADC_AUX_ST_ADDR_OFFSET                0x014
#define SADC_PCM_DAT_ADDR_OFFSET               0x018
#define SADC_SADC_CALIB_DATIN_ADDR_OFFSET      0x01c
#define SADC_SADC_CALIB_DATSEL_ADDR_OFFSET     0x020
#define SADC_SADC_CALIB_DATOUT_ADDR_OFFSET     0x024
#define SADC_SADC_ANA_CTRL_ADDR_OFFSET         0x028
#define SADC_MIC_CTRL_ADDR_OFFSET              0x02c

//================================
//BLOCK SADC reg struct define
typedef union //0x000
{
    struct
    {
        uint32_t SADC_SOC:                           1; // bit0
        uint32_t SADC_CALIB_MODE:                    1; // bit1
        uint32_t SADC_CONV_MODE:                     1; // bit2
        uint32_t SADC_DMAC_EN:                       1; // bit3
        uint32_t SADC_CLK_PH:                        1; // bit4
        uint32_t SADC_SAMP_MOD:                      2; // bit[6:5] ---
                                                        // 2'b00: software mode(normal mode)
                                                        // 2'b01: RSSI sample mode( ADC controlled by BB)
                                                        // 2'b10: decimation filter mode ( sample vioce data, must enable DMA)
                                                        // 2'b11: SOC is from ADTIM adctrg
        uint32_t SADC_DECIM_END:                     1; // bit7     --- write as 1 to stop decimation filter
        uint32_t SADC_RSSI_SAMP_DLY:                 6; // bit[13:8]--- delay time from rssi_req to SADC sample RSSI
                                                        // unit: one 16M clock period
                                                        // range: 0 ~ 4us
        uint32_t SADC_DBG_CTRL:                      1; // bit14    ---
                                                        // 0: normal mode
                                                        // 1: chip is configed as SADC analog IP
                                                        // the SOC & CLK is from external chip
        uint32_t RSV_NOUSE1:                         1; // bit15    --- Ignore me
        uint32_t SADC_HPF_COEF:                      4; // bit[19:16]-- decimation filter high pass filter coef
                                                        // 0: high pass filter is bypass
                                                        // 1~12: y(n)= (1-2^-sadc_hpf_coef)*y(n-1) + x(n) - x(n-1)
        uint32_t SADC_AUX_CLK_DIV:                  10; // bit[29:20]-- adc function clock = hclk / (sadc_aux_clk_div + 1 )
                                                        // note
                                                        // 1: make sure adc function clock is not more than 16M
                                                        // 2: when sadc_samp_mod is 2'b10 ( decimation filter mode)
                                                        // please makesure adc function clock is 16M
        uint32_t RSV_END:                            2; // bit[31:30]
    };
    uint32_t Word;
} SADC_CTRL_TypeDef; //0x000


//================================
#define SADC_SADC_SOC_POS                   0
#define SADC_SADC_CALIB_MODE_POS            1
#define SADC_SADC_CONV_MODE_POS             2
#define SADC_SADC_DMAC_EN_POS               3
#define SADC_SADC_CLK_PH_POS                4
#define SADC_SADC_SAMP_MOD_LSB              5
#define SADC_SADC_SAMP_MOD_WIDTH            2
#define SADC_SADC_DECIM_END_POS             7
#define SADC_SADC_RSSI_SAMP_DLY_LSB         8
#define SADC_SADC_RSSI_SAMP_DLY_WIDTH       6
#define SADC_SADC_DBG_CTRL_POS              14
#define SADC_SADC_HPF_COEF_LSB              16
#define SADC_SADC_HPF_COEF_WIDTH            4
#define SADC_SADC_AUX_CLK_DIV_LSB           20
#define SADC_SADC_AUX_CLK_DIV_WIDTH         10
//================================

typedef union //0x004 
{
    struct
    {
        uint32_t AUTO_SW_CH:                         1; // bit0     --- when is 1
                                                        // ADC_VIN_SEL is auto change
                                                        // only used when sadc_samp_mod is 2'b11
        uint32_t AUTO_CH_NUM:                        3; // bit[3:1] --- swith order:
        uint32_t RSV_END:                           28; // bit[31:4]
    };
    uint32_t Word;
} SADC_AUTO_SW_CTRL_TypeDef; //0x004


//================================
#define SADC_AUTO_SW_CH_POS                 0
#define SADC_AUTO_CH_NUM_LSB                1
#define SADC_AUTO_CH_NUM_WIDTH              3
//================================

typedef union //0x008 
{
    struct
    {
        uint32_t SADC_CH_SET0:                       4; // bit[3:0] --- when auto_sw_ch is 0
                                                        // SADC_VIN_SEL is sadc_ch_set0
                                                        // 0: VDD33/2
                                                        // 1: PAD_VBK
                                                        // 2: PAD_VDD12
                                                        // 3: PGAOUT
                                                        // 4: MICIN
                                                        // 5~14: ADCIN0 ~ ADCIN9
                                                        // 15: RF
                                                        //    RF.RF_RSV[0] : RF TEMP
                                                        //    RF.RF_RSV[1] : RF BANDGAP
                                                        //    RF.RF_RSV[2] : RF RSSI
                                                        //    RF.RF_RSV[3] : RF VDD_IF 
        uint32_t SADC_CH_SET1:                       4; // bit[7:4]
        uint32_t SADC_CH_SET2:                       4; // bit[11:8]
        uint32_t SADC_CH_SET3:                       4; // bit[15:12]
        uint32_t SADC_CH_SET4:                       4; // bit[19:16]
        uint32_t SADC_CH_SET5:                       4; // bit[23:20]
        uint32_t SADC_CH_SET6:                       4; // bit[27:24]
        uint32_t SADC_CH_SET7:                       4; // bit[31:28]
    };
  uint32_t Word;
} SADC_CH_CTRL_TypeDef; //0x008 


//================================
#define SADC_SADC_CH_SET0_LSB               0
#define SADC_SADC_CH_SET0_WIDTH             4
#define SADC_SADC_CH_SET1_LSB               4
#define SADC_SADC_CH_SET1_WIDTH             4
#define SADC_SADC_CH_SET2_LSB               8
#define SADC_SADC_CH_SET2_WIDTH             4
#define SADC_SADC_CH_SET3_LSB               12
#define SADC_SADC_CH_SET3_WIDTH             4
#define SADC_SADC_CH_SET4_LSB               16
#define SADC_SADC_CH_SET4_WIDTH             4
#define SADC_SADC_CH_SET5_LSB               20
#define SADC_SADC_CH_SET5_WIDTH             4
#define SADC_SADC_CH_SET6_LSB               24
#define SADC_SADC_CH_SET6_WIDTH             4
#define SADC_SADC_CH_SET7_LSB               28
#define SADC_SADC_CH_SET7_WIDTH             4
//================================

typedef union //0x00c
{
    struct
    {
        uint32_t SADC_AUX_FLG:                       1; // bit0
        uint32_t SADC_FILTER_BUSY:                   1; // bit1
        uint32_t SADC_AUX_FLG_CLR:                   1; // bit2
        uint32_t RSV_END:                           29; // bit[31:3]
    };
    uint32_t Word;
} SADC_STCTRL_TypeDef; //0x00c


//================================
#define SADC_SADC_AUX_FLG_POS               0
#define SADC_SADC_FILTER_BUSY_POS           1
#define SADC_SADC_AUX_FLG_CLR_POS           2
//================================

typedef union //0x014 
{
    struct
    {
        uint32_t SADC_AUX_DOUT:                     10; // bit[9:0]
        uint32_t RSV_NOUSE1:                         2; // bit[11:10] --- Ignore me
        uint32_t SADC_CH_ST:                         4; // bit[15:12]
        uint32_t RSV_END:                           16; // bit[31:16]
    };
    uint32_t Word;
} SADC_AUX_ST_TypeDef; //0x014


//================================
#define SADC_SADC_AUX_DOUT_LSB              0
#define SADC_SADC_AUX_DOUT_WIDTH            10
#define SADC_SADC_CH_ST_LSB                 12
#define SADC_SADC_CH_ST_WIDTH               4
//================================

typedef union //0x028 
{
    struct
    {
        uint32_t SADC_EN:                            1; // bit0
        uint32_t SADC_VREF_TRIM:                     2; // bit[2:1]   ---
                                                        // 0: 1.2V
                                                        // 1: 1.8V
                                                        // 2: 2.4V
                                                        // 3: 3V
        uint32_t SADC_IBSEL_CMP:                     3; // bit[5:3]   ---
                                                        // 000: 15uA
                                                        // 111: 105uA
                                                        // 15uA/step
        uint32_t SADC_IBSEL_VCM:                     3; // bit[8:6]   ---
                                                        // 000: 15uA
                                                        // 111: 105uA
                                                        // 15uA/step
        uint32_t SADC_IBSEL_VREF:                    3; // bit[11:9]  ---
                                                        // 000: 15uA
                                                        // 111: 105uA
                                                        // 15uA/step
        uint32_t SADC_IBSEL_BUF:                     3; // bit[14:12] ---
                                                        // 000: 15uA
                                                        // 111: 105uA
                                                        // 15uA/step
        uint32_t SADC_VREF_SEL:                      1; // bit15      ---
                                                        // 0: ref is from VDDA
                                                        // 1: ref is gen from VBG
        uint32_t SADC_CALCAP_SEL:                    2; // bit[17:16] ---
                                                        // 01: most precision but narrow range
                                                        // 11: widest range but less precision
        uint32_t SADC_CAL_OS:                        2; // bit[19:18] ---
                                                        // 01: single ended tie to GND
                                                        // 01: single ended tie to VREF
        uint32_t SADC_INBUF_BYPSS:                   1; // bit20      ---
                                                        // 0: through input buffer
                                                        // 1: bypass input buffer
        uint32_t RSV_END:                           11; // bit[31:21]
    };
    uint32_t Word;
} SADC_SADC_ANA_CTRL_TypeDef; //0x028


//================================
#define SADC_SADC_EN_POS                    0
#define SADC_SADC_VREF_TRIM_LSB             1
#define SADC_SADC_VREF_TRIM_WIDTH           2
#define SADC_SADC_IBSEL_CMP_LSB             3
#define SADC_SADC_IBSEL_CMP_WIDTH           3
#define SADC_SADC_IBSEL_VCM_LSB             6
#define SADC_SADC_IBSEL_VCM_WIDTH           3
#define SADC_SADC_IBSEL_VREF_LSB            9
#define SADC_SADC_IBSEL_VREF_WIDTH          3
#define SADC_SADC_IBSEL_BUF_LSB             12
#define SADC_SADC_IBSEL_BUF_WIDTH           3
#define SADC_SADC_VREF_SEL_POS              15
#define SADC_SADC_CALCAP_SEL_LSB            16
#define SADC_SADC_CALCAP_SEL_WIDTH          2
#define SADC_SADC_CAL_OS_LSB                18
#define SADC_SADC_CAL_OS_WIDTH              2
#define SADC_SADC_INBUF_BYPSS_POS           20
//================================

typedef union //0x02c 
{
    struct
    {
        uint32_t MIC_MODE_SEL:                       1; // bit0     --- Analog microphone connect mode selected
                                                        // 1: connect without cap and res
                                                        // 0: connect with coupling cap
        uint32_t MIC_PD:                             1; // bit1     --- Micbias powerdown signal
        uint32_t MIC_TRIM:                           7; // bit[8:2] --- Micbias output voltage trimming signal
        uint32_t MIC_VREF_SEL:                       1; // bit9     --- Micbias Internal voltage reference selected
                                                        // 0: 750mV
                                                        // 1: 1.5V
        uint32_t PGA_BIAS_SEL:                       2; // bit[11:10]-- PGA enable signal, high active
        uint32_t PGA_EN:                             1; // bit 12
        uint32_t PGA_VOL:                            5; // bit[17:13]
        uint32_t PGA_VREF_SEL:                       1; // bit18     -- pga Internal voltage reference selected
                                                        // 0: 750mV
                                                        // 1: 1.5V
        uint32_t RSV_END:                           13; // bit[31:19]
    };
  uint32_t Word;
} SADC_MIC_CTRL_TypeDef; //0x024


//================================
//#define SADC_MIC_MODE_SEL_POS               0
//#define SADC_MIC_PD_POS                     1
//#define SADC_MIC_TRIM_LSB                   2
//#define SADC_MIC_TRIM_WIDTH                 7
//#define SADC_MIC_VREF_SEL_POS               9
//#define SADC_PGA_BIAS_SEL_LSB               10
//#define SADC_PGA_BIAS_SEL_WIDTH             2
//#define SADC_PGA_EN_POS                     12
//#define SADC_PGA_VOL_LSB                    13
//#define SADC_PGA_VOL_WIDTH                  5
//#define SADC_PGA_VREF_SEL_POS               18
//================================

//================================
//BLOCK SADC top struct define 
typedef struct
{
    __IO  SADC_CTRL_TypeDef                      CTRL                ; // 0x000,
    __IO  SADC_AUTO_SW_CTRL_TypeDef              AUTO_SW_CTRL        ; // 0x004,
    __IO  SADC_CH_CTRL_TypeDef                   CH_CTRL             ; // 0x008,
    __IO  SADC_STCTRL_TypeDef                    STCTRL              ; // 0x00c,
    __IO  uint32_t                               DC_OFFSET           ; // 0x010,
                                                                       // DC offset
                                                                       // final out = ana_out + sadc_dc_off
                                                                       // is sadc_dc_off is signed data
                                                                       // data arrange : -8 ~ 7 for software or RSSI function
                                                                       //  command value for decimation is 10'h200
    __I   SADC_AUX_ST_TypeDef                    AUX_ST              ; // 0x014,
    __I   uint32_t                               PCM_DAT             ; // 0x018,
    __IO  uint32_t                               SADC_CALIB_DATIN    ; // 0x01c,
    __IO  uint32_t                               SADC_CALIB_DATSEL   ; // 0x020,
    __I   uint32_t                               SADC_CALIB_DATOUT   ; // 0x024,
    __IO  SADC_SADC_ANA_CTRL_TypeDef             SADC_ANA_CTRL       ; // 0x028,
    __IO  SADC_MIC_CTRL_TypeDef                  MIC_CTRL            ; // 0x02c,
} SADC_TypeDef;


#define SADC  (( SADC_TypeDef  *)     SADC_BASE)

#endif
