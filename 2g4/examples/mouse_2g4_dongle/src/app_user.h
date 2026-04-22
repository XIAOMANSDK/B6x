#ifndef APP_USER_H_
#define APP_USER_H_

#include <stdint.h>

/*
 * DEFINES
 ****************************************************************************************
 */
#ifndef MOUSE_LEN
#define MOUSE_LEN              (4)
#endif
/*
 * VARIABLE DECLARATION
 ****************************************************************************************
 */

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

void usbdInit(void);

void usbd_wakeup(void);

uint8_t usbd_mouse_report(void);

uint8_t *get_mouse_pkt(void);

void clear_mouse_pkt(void);

#endif // APP_USER_H_
