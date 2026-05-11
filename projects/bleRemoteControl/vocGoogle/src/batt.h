#ifndef _BATT_H_
#define _BATT_H_

#include <stdint.h>

#define BATT_LVL_MAX   100
#define BATT_LVL_MIN   0

void batt_init(void);

uint8_t batt_level(void);

#endif  //_BATT_H_
