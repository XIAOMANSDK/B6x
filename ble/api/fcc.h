#ifndef B6X_FCC_H_
#define B6X_FCC_H_

#include <stdint.h>

void fcc_init(void);

void fcc_stop(void);

/// freq_idx:0 ~ 39(2402 ~ 2480)
/// Tx调制数据
void fcc_tx_mod(uint8_t freq_idx);

/// Rx调制数据
void fcc_rx_mod(uint8_t freq_idx);

/// Tx单载波(定频)
void fcc_tx_carr(uint8_t freq_idx);

/// Rx单载波(定频)
void fcc_rx_carr(uint8_t freq_idx);

#endif // B6X_FCC_H_
