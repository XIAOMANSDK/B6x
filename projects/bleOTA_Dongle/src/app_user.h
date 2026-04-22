#ifndef APP_USER_H
#define APP_USER_H

#include <stdint.h>

#define APP_INIT_TIMEOUT 30

void user_init(void);
void user_procedure(void);

uint8_t *get_scan_name(uint8_t *len);
uint8_t  get_auto_conn(void);
uint8_t  get_scan_duration(void);

void             scan_list_reset(void);
const struct gap_bdaddr *scan_list_get(void);

void init_timer_stop(void);
void init_timer_start(void);
void ota_ok_wait_next(void);

void disc_ota_init(uint8_t conidx);
#endif // APP_USER_H
