#ifndef _BTMR_H_
#define _BTMR_H_


#include "reg_btmr.h"


#define TIME_OUT_FLAG (BTMR->RIF)
#define CLR_UI_FLAG() do { BTMR->ICR = 1; } while (0)

// timeout = arr * 10us
void timeOutMsInit(uint16_t arr);


//void btmrConfig(void);

//void bootDelayUs(uint32_t us);

//void bootDelayMs(uint32_t ms);
//void btmr_delay(uint16_t tpsc, uint16_t tcnt);
void tick_delay(uint16_t tpsc, uint16_t tcnt);
#include "rom.h"
#define btmrConfig()
#define bootDelayMs(time)  btmr_delay(16000, time)


#endif
