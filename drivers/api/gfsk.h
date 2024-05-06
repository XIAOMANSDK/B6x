#ifndef RF_GFSK_H_
#define RF_GFSK_H_

#include <stdint.h>

void rf_gfsk_init(void);

//tx_freq_mhz:2402 ~ 2480(step 2)
void rf_gfsk_tx_freq(uint16_t tx_freq_mhz);

#endif // RF_GFSK_H_
