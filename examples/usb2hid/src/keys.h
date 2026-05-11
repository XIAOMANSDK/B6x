#ifndef KEYS_H_
#define KEYS_H_

#if (USE_KEYS)
#include "drvs.h"
#include "dbg.h"

#define LED1    (1 << 8)
#define LED2    (1 << 9)
#define LEDS    (LED2 | LED1)

#define PA_BTN1 15
#define BTN1    (1 << PA_BTN1)
#define PA_BTN2 16
#define BTN2    (1 << PA_BTN2)
#define PA_BTN3 17
#define BTN3    (1 << PA_BTN3)
#define BTNS    (BTN1 | BTN2 | BTN3)

static __inline void keys_init(void)
{
    GPIO_DIR_CLR(BTNS);
    iom_ctrl(PA_BTN1, IOM_INPUT | IOM_PULLUP | IOM_SEL_GPIO);
    iom_ctrl(PA_BTN2, IOM_INPUT | IOM_PULLUP | IOM_SEL_GPIO);
    iom_ctrl(PA_BTN3, IOM_INPUT | IOM_PULLUP | IOM_SEL_GPIO);
    
    GPIO_DIR_SET_HI(LEDS);
}
#else

#define keys_init()         // empty

#endif // (USE_KEYS)

#endif // KEYS_H_
