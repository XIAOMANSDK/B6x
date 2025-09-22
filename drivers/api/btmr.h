#ifndef _BTMR_H_
#define _BTMR_H_

#include <stdint.h>

#if (ROM_UNUSED)
void btmr_delay(uint16_t tpsc, uint16_t tcnt);
#endif
void tick_delay(uint16_t tpsc, uint16_t tcnt);

#endif
