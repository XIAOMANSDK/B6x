#ifndef APP_USER_H_
#define APP_USER_H_

#include <stdint.h>
#define KBD_IN_EP                 0x81
#define RAW_IN_EP                 0x82
#define RAW_IN_EP_SIZE            64

/*
 * VARIABLE DECLARATION
 ****************************************************************************************
 */
extern volatile uint16_t pkt_sidx, pkt_eidx;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

void usbdInit(void);

void usbd_wakeup(void);

void init_timer_start(void);

void init_timer_stop(void);

void usbd_kb_report(uint8_t len, const uint8_t *data);
void usbd_mic_report(void);
void usbd_mic_push(const uint8_t *apcm);
uint8_t *get_kb_pkt(void);
uint8_t *get_mic_pkt(void);

#endif // APP_USER_H_
