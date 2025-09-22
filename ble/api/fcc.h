#ifndef B6X_FCC_H_
#define B6X_FCC_H_

#include <stdint.h>
typedef struct 
{
    uint8_t tx_power;
    uint8_t cap;
    uint8_t PLL_DAC_ADJ00;//2402
    uint8_t PLL_DAC_ADJ12;//2440
    uint8_t PLL_DAC_ADJ05;//2480
    uint8_t vco_adj;
    uint8_t bg_res_trim;//BG_RES_TRIM;
    uint8_t rev2;
    
}Fcc_Config_t,*Fcc_Config_p;
void fcc_init(void);

void fcc_stop(void);

/// freq_idx:0 ~ 39(2402 ~ 2480)
/// Tx调制数据
void fcc_tx_mod(uint8_t freq_idx);
/// pa_target:0x00~0x0F, default:0x0C
void fcc_tx_mod_pa(uint8_t freq_idx, uint8_t pa_target);

/// Rx调制数据
void fcc_rx_mod(uint8_t freq_idx);
///vco_adj:0x00~0x07,default:0x06
void fcc_rx_mod_vco(uint8_t freq_idx, uint8_t vco_adj);

/// Tx单载波(定频)
void fcc_tx_carr(uint8_t freq_idx);
/// pa_target:0x00~0x0F, default:0x0C
void fcc_tx_carr_pa(uint8_t freq_idx, uint8_t pa_target);

/// Rx单载波(定频)
void fcc_rx_carr(uint8_t freq_idx);
///vco_adj:0x00~0x07,default:0x06
void fcc_rx_carr_vco(uint8_t freq_idx, uint8_t vco_adj);

void rf_dac_tab0_set(uint32_t dac_tab0);

void rf_dac_tab1_set(uint32_t dac_tab1);

void rf_dac_tab2_set(uint32_t dac_tab2);

#endif // B6X_FCC_H_
