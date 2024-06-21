/**
 ****************************************************************************************
 *
 * @file sadc.h
 *
 * @brief Header file - SADC(ADC) Driver
 *
 ****************************************************************************************
 */

#ifndef _SADC_H_
#define _SADC_H_

#include <stdint.h>


/*
 * DEFINES
 ****************************************************************************************
 */

enum sadc_chnl_set
{
    // 0~4: Special Power Pads
    SADC_CH_VDD33           = 0,  // VDD33/2
    SADC_CH_VBK             = 1,  // PAD_VBK
    SADC_CH_VDD12           = 2,  // PAD_VDD12
    SADC_CH_PGAOUT          = 3,
    SADC_CH_MICIN           = 4,
    
    // 5~14: ADCIN0 ~ ADCIN9(Single ended channel)
    SADC_CH_AIN0            = 5,  // PA00 or PA11
    SADC_CH_AIN1            = 6,  // PA01 or PA12
    SADC_CH_AIN2            = 7,  // PA02 or PA13
    SADC_CH_AIN3            = 8,  // PA04
    SADC_CH_AIN4            = 9,  // PA05 or PA14
    SADC_CH_AIN5            = 10, // PA06 or PA15
    SADC_CH_AIN6            = 11, // PA07 or PA16
    SADC_CH_AIN7            = 12, // PA08
    SADC_CH_AIN8            = 13, // PA09
    SADC_CH_AIN9            = 14, // PA10
    
    // 15: RF RSSI sample
    SADC_CH_RFRSV           = 15, // RF.RF_RSV[3:0]
};

enum sadc_rf_rsv
{
    SADC_RF_TEMP            = 0x1, // 4'b0001
    SADC_RF_BANDGAP         = 0x2, // 4'b0010
    SADC_RF_RSSI            = 0x4, // 4'b0100
    SADC_RF_VDDIF           = 0x8, // 4'b1000
};

enum sadc_ana_ctrl
{
    // bit0 - SADC Enable
    SADC_EN_POS             = 0,
    SADC_EN_BIT             = (1 << SADC_EN_POS),
    
    // bit[2:1] - Vref Trim
    SADC_VREF_TRIM_LSB      = 1,
    SADC_VREF_TRIM_1V2      = (0 << SADC_VREF_TRIM_LSB),
    SADC_VREF_TRIM_1V8      = (1 << SADC_VREF_TRIM_LSB),
    SADC_VREF_TRIM_2V4      = (2 << SADC_VREF_TRIM_LSB),
    SADC_VREF_TRIM_3V0      = (3 << SADC_VREF_TRIM_LSB),
    
    // bit[5:3] - ibsel cmp 15uA~105uA, step in 15uA
    SADC_IBSEL_CMP_LSB      = 3,
    SADC_IBSEL_CMP_015UA    = (0 << SADC_IBSEL_CMP_LSB),
    SADC_IBSEL_CMP_105UA    = (7 << SADC_IBSEL_CMP_LSB),
    // bit[8:6] - ibsel vcm 15uA~105uA, step in 15uA
    SADC_IBSEL_VCM_LSB      = 6,
    SADC_IBSEL_VCM_015UA    = (0 << SADC_IBSEL_VCM_LSB),
    SADC_IBSEL_VCM_105UA    = (7 << SADC_IBSEL_VCM_LSB),
    // bit[11:9] - ibsel Vref 15uA~105uA, step in 15uA
    SADC_IBSEL_VREF_LSB     = 9,
    SADC_IBSEL_VREF_015UA   = (0 << SADC_IBSEL_VREF_LSB),
    SADC_IBSEL_VREF_105UA   = (7 << SADC_IBSEL_VREF_LSB),
    // bit[14:12] - ibsel buf 15uA~105uA, step in 15uA
    SADC_IBSEL_BUF_LSB      = 12,
    SADC_IBSEL_BUF_015UA    = (0 << SADC_IBSEL_BUF_LSB),
    SADC_IBSEL_BUF_105UA    = (7 << SADC_IBSEL_BUF_LSB),
    
    // bit15 - SADC Vref gen from VBG or VDDA
    SADC_VREF_POS           = 15,
    SADC_VREF_VDD           = (0 << SADC_VREF_POS),
    SADC_VREF_VBG           = (1 << SADC_VREF_POS),
    
    // bit[17:16] - SADC precision or range
    SADC_CALCAP_LSB         = 16,
    SADC_CALCAP_PRECISION   = (1 << SADC_CALCAP_LSB),
    SADC_CALCAP_RANGE       = (3 << SADC_CALCAP_LSB),
    // bit[19:18] - SADC single ended tie to GND or VREF
    SADC_CALOS_POS          = 18,
    SADC_CALOS_GND          = (0 << SADC_CALOS_POS),
    SADC_CALOS_VREF         = (1 << SADC_CALOS_POS),
    
    // bit20 - SADC bypass input buffer
    SADC_INBUF_BYPSS_POS    = 20,
    SADC_INBUF_BYPSS_BIT    = (1 << SADC_INBUF_BYPSS_POS),
};

#define SADC_IBSEL_CMP(sel)   ((sel) << SADC_IBSEL_CMP_LSB)
#define SADC_IBSEL_VCM(sel)   ((sel) << SADC_IBSEL_VCM_LSB)
#define SADC_IBSEL_VREF(sel)  ((sel) << SADC_IBSEL_VREF_LSB)
#define SADC_IBSEL_BUF(sel)   ((sel) << SADC_IBSEL_BUF_LSB)

#define SADC_ANA_DFLT         (SADC_VREF_VBG | SADC_CALCAP_PRECISION | SADC_IBSEL_BUF(3) | SADC_IBSEL_VREF(3)  \
                               | SADC_IBSEL_VCM(3) | SADC_IBSEL_CMP(3) | SADC_VREF_TRIM_2V4 | SADC_EN_BIT)

#define SADC_ANA_VREF_1V2     (SADC_VREF_VBG | SADC_CALCAP_PRECISION | SADC_IBSEL_BUF(3) | SADC_IBSEL_VREF(3)  \
                               | SADC_IBSEL_VCM(3) | SADC_IBSEL_CMP(3) | SADC_VREF_TRIM_1V2 | SADC_EN_BIT)

#define SADC_ANA_VREF_2V4     (SADC_VREF_VBG | SADC_CALCAP_PRECISION | SADC_IBSEL_BUF(3) | SADC_IBSEL_VREF(3)  \
                               | SADC_IBSEL_VCM(3) | SADC_IBSEL_CMP(3) | SADC_VREF_TRIM_2V4 | SADC_EN_BIT)

#define SADC_ANA_VREF_VDD     (SADC_VREF_VDD | SADC_CALCAP_PRECISION | SADC_IBSEL_BUF(3) | SADC_IBSEL_VREF(3)  \
                               | SADC_IBSEL_VCM(3) | SADC_IBSEL_CMP(3) | SADC_EN_BIT)

enum sadc_mic_sel
{
    // bit0 - Analog microphone connect mode
    SADC_MIC_MODE_POS       = 0,
    SADC_MIC_MODE_CAP       = (0 << SADC_MIC_MODE_POS),
    SADC_MIC_MODE_NO_CAP    = (1 << SADC_MIC_MODE_POS),
    // bit1 - Micbias powerdown signal
    SADC_MIC_PWD_POS        = 1,
    SADC_MIC_PWD_BIT        = (1 << SADC_MIC_PWD_POS),
    
    // bit[8:2] - Micbias output voltage trimming signal
    SADC_MIC_TRIM_LSB       = 2,
    SADC_MIC_TRIM_MSK       = (0x7F << SADC_MIC_TRIM_LSB),
    // bit9 - Micbias Internal voltage reference(0:750mV, 1:1.5V) 
    SADC_MIC_VREF_POS       = 9,
    SADC_MIC_VREF_750MV     = (0 << SADC_MIC_VREF_POS),
    SADC_MIC_VREF_1V5       = (1 << SADC_MIC_VREF_POS),
    
    // bit[11:10] - pga voltage trimming signal
    SADC_PGA_BIAS_LSB       = 10,
    SADC_PGA_BIAS_MSK       = (0x03 << SADC_PGA_BIAS_LSB),
    // bit12 - pga enable signal
    SADC_PGA_EN_POS         = 12,
    SADC_PGA_EN_BIT         = (1 << SADC_PGA_EN_POS),
    // bit[17:13] - pga voltage trimming signal
    SADC_PGA_VOL_LSB        = 13,
    SADC_PGA_VOL_MSK        = (0x1F << SADC_PGA_VOL_LSB),
    // bit18 - pga Internal voltage reference(0:750mV, 1:1.5V)
    SADC_PGA_VREF_POS       = 18,
    SADC_PGA_VREF_750MV     = (0 << SADC_PGA_VREF_POS),
    SADC_PGA_VREF_1V5       = (1 << SADC_PGA_VREF_POS),
};

#define SADC_MIC_TRIM(val)    ((val) << SADC_MIC_TRIM_LSB)
#define SADC_PGA_BIAS(val)    ((val) << SADC_PGA_BIAS_LSB)
#define SADC_PGA_VOL(val)     ((val) << SADC_PGA_VOL_LSB)

// Micbias output voltage = SADC_MIC_VREF_POS(voltage)*1.5
//                        = 0.75V*1.5 = 1.125V
//                        = 1.5V *1.5 = 2.25V (default)(2.25V <= VDD33) @The chip voltage is equal to the voltage on the VDD33 pin.
//                                    = VDD33 (default)(2.25V > VDD33)  @The chip voltage is equal to the voltage on the VDD33 pin.
// Micin voltage select   = SADC_PGA_VREF_POS(voltage) 
//                        = 750mV                      @microphone output 0.7V.  Need SADC_VREF_TRIM_1V2.
//                        = 1.5V (default)             @microphone output 1.5V.  Need SADC_VREF_TRIM_2V4(2.4V < VDD33).
// .MIC_PD=0, .MIC_VREF_SEL=1, .PGA_BIAS_SEL=1, .PGA_EN=1
#define SADC_MIC_DFLT         (SADC_PGA_VOL(0x1B) | SADC_PGA_BIAS(1) | SADC_MIC_TRIM(0x40) \
                                | SADC_PGA_EN_BIT | SADC_PGA_VREF_1V5 | SADC_MIC_VREF_1V5) // 0x36502->0x77700

enum sadc_ctrl_bfs
{
    // bit0 - start of conversion
    SADC_CR_SOC_POS         = 0,
    SADC_CR_SOC_BIT         = (1 << SADC_CR_SOC_POS),
    // bit1 - calibration mode
    SADC_CR_CALIB_POS       = 1,
    SADC_CR_CALIB_BIT       = (1 << SADC_CR_CALIB_POS),
    
    // bit2 - conversion mode(0-single, 1-continue)
    SADC_CR_CONV_MODE_POS   = 2,
    SADC_CR_CONV_MODE_BIT   = (1 << SADC_CR_CONV_MODE_POS),
    SADC_CR_CONV_SINGLE     = (0 << SADC_CR_CONV_MODE_POS),
    SADC_CR_CONV_CONTINUE   = (1 << SADC_CR_CONV_MODE_POS),
    
    // bit3 - DMA enable
    SADC_CR_DMAEN_POS       = 3,
    SADC_CR_DMAEN_BIT       = (1 << SADC_CR_DMAEN_POS),
    
    // bit4 - conversion at clk phase(0-negedge, 1-posedge)
    SADC_CR_CLKPH_POS       = 4,
    SADC_CR_CLKPH_NEGEDGE   = (0 << SADC_CR_CLKPH_POS),
    SADC_CR_CLKPH_POSEDGE   = (1 << SADC_CR_CLKPH_POS),
    
    // bit[6:5] - Sample mode
    SADC_CR_SAMP_MODE_LSB   = 5,
    SADC_CR_SAMP_MODE_MSK   = (0x03 << SADC_CR_SAMP_MODE_LSB),
    SADC_CR_SAMP_SOFT       = (0 << SADC_CR_SAMP_MODE_LSB), // software mode(normal mode)
    SADC_CR_SAMP_RSSI       = (1 << SADC_CR_SAMP_MODE_LSB), // RSSI sample mode( ADC controlled by BB)
    SADC_CR_SAMP_PCM        = (2 << SADC_CR_SAMP_MODE_LSB), // sample vioce data, must enable DMA
    SADC_CR_SAMP_ADTMR      = (3 << SADC_CR_SAMP_MODE_LSB), // SOC is from ADTIM adctrg

    // bit7 - write as 1 to stop decimation filter
    SADC_CR_PCMEND_POS      = 7,
    SADC_CR_PCMEND_BIT      = (1 << SADC_CR_PCMEND_POS),

    // bit[13:8] - delay time from rssi_req to SADC sample RSSI
    SADC_CR_RSSI_DLY_LSB    = 8,
    SADC_CR_RSSI_DLY_MSK    = (0x3F << SADC_CR_RSSI_DLY_LSB),
    
    // bit14 - DBG enable, configed as SADC analog IP
    SADC_CR_DBGEN_POS       = 14,
    SADC_CR_DBGEN_BIT       = (1 << SADC_CR_DBGEN_POS),
    
    // bit[19:16] - decimation filter high pass filter coef
    SADC_CR_HPF_COEF_LSB    = 16,
    SADC_CR_HPF_COEF_MSK    = (0x0F << SADC_CR_HPF_COEF_LSB),
    // bit[29:20] - adc function clock = hclk / (sadc_aux_clk_div + 1)
    SADC_CR_CLK_DIV_LSB     = 20,
    SADC_CR_CLK_DIV_MSK     = (0x3FF << SADC_CR_CLK_DIV_LSB),
};

#define SADC_CR_HPF(coef)     ((coef) << SADC_CR_HPF_COEF_LSB)
#define SADC_CR_CLK(div)      ((div) << SADC_CR_CLK_DIV_LSB)

#define SADC_CLK_DIV         (((SYS_CLK + 1) * 4) - 1) // div_4M
#define SADC_CR_DFLT         (SADC_CR_CLK(SADC_CLK_DIV) | SADC_CR_HPF(11) | SADC_CR_CLKPH_POSEDGE \
                             | SADC_CR_SAMP_SOFT | SADC_CR_CONV_SINGLE)

/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Init SADC Module.
 *
 * @param[in] ana_ctrl   Bits field of value @see enum sadc_ana_ctrl.
 ****************************************************************************************
 */
void sadc_init(uint32_t ana_ctrl);

/**
 ****************************************************************************************
 * @brief Deinit SADC Module.
 *
 ****************************************************************************************
 */
void sadc_deinit(void);

/**
 ****************************************************************************************
 * @brief Config SADC Module.
 *
 * @param[in] ctrl   Bits field of value @see enum sadc_ctrl_bfs.
 ****************************************************************************************
 */
void sadc_conf(uint32_t ctrl);

/**
 ****************************************************************************************
 * @brief SADC read data.
 *
 * @param[in] chset  Bits field of value @see enum sadc_chnl_set.
 * @param[in] times  Number of times sadc read.
 *
 * @return Multiple(times) averages.
 ****************************************************************************************
 */
uint16_t sadc_read(uint8_t chset, uint16_t times);

/**
 ****************************************************************************************
 * @brief Config SADC Support DMA.
 *
 * @param[in] sw_auto  SADC auto chnl.
 * @param[in] ch_ctrl  Bits field of value @see enum sadc_chnl_set.
 ****************************************************************************************
 */
void sadc_dma(uint8_t sw_auto, uint32_t ch_ctrl);

/**
 ****************************************************************************************
 * @brief Config SADC Support PCM.
 *
 * @param[in] mic_sel  Bits field of value @see enum sadc_mic_sel.
 ****************************************************************************************
 */
void sadc_pcm(uint32_t mic_sel);

/**
 ****************************************************************************************
 * @brief Config SADC Support ATMR.
 *
 * @param[in] sw_auto  SADC auto chnl.
 * @param[in] ch_ctrl  Bits field of value @see enum sadc_chnl_set.
 ****************************************************************************************
 */
void sadc_atmr(uint8_t sw_auto, uint32_t ch_ctrl);

/**
 ****************************************************************************************
 * @brief Random Number Generated by SADC.
 *
 * @return Random Number.
 ****************************************************************************************
 */
uint32_t sadc_rand_num(void);
#endif // _SADC_H_
