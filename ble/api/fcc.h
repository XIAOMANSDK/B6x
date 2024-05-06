#ifndef B6X_FCC_H_
#define B6X_FCC_H_

#include <stdint.h>

void fcc_init(void);

void fcc_stop(void);

/// freq_idx:0 ~ 39(2402 ~ 2480)
/// Tx��������
void fcc_tx_mod(uint8_t freq_idx);

/// Rx��������
void fcc_rx_mod(uint8_t freq_idx);

/// Tx���ز�(��Ƶ)
void fcc_tx_carr(uint8_t freq_idx);

/// Rx���ز�(��Ƶ)
void fcc_rx_carr(uint8_t freq_idx);

#endif // B6X_FCC_H_
