/**
 ****************************************************************************************
 *
 * @file keys.h
 *
 * @brief Header file - Keys Scanning and Report
 *
 ****************************************************************************************
 */

#ifndef _KEYS_H_
#define _KEYS_H_

#include <stdint.h>
#include <stdbool.h>
#include "hidkey.h"

#define KEY_LONG_PRESS_TIME_MS   (3000) /*!< 所有组合键长按时间,单位ms */

#define KEY_MULTI_MAX_NUM           (3) /*!< 组合键个数 */

// 按小米国内按键实际布局重命名
enum {
    KEY_IDX_NO = 0x0000,
    KEY_IDX_POWER = KEY_POWER,
    KEY_IDX_VOICE = KEY_F5,
    KEY_IDX_UP = MKEY_MENU_UP,
    KEY_IDX_DOWN = MKEY_MENU_DN,
    KEY_IDX_LEFT = MKEY_MENU_LEFT,
    KEY_IDX_RIGHT = MKEY_MENU_RIGHT,
    KEY_IDX_ENTER = MKEY_MENU_PICK,
    KEY_IDX_HOME = KEY_HOME,
    KEY_IDX_BACK = MKEY_WWW_BACK,
    KEY_IDX_MENU = KEY_APP,
    KEY_IDX_VOLUP = KEY_VOL_UP,
    KEY_IDX_VOLDN = KEY_VOL_DN,

    KEY_IDX_INVALID = 0xFF
};

typedef struct
{
    uint16_t keycode[2];
    const char *info;
    void (*proc_cb)(void);
} double_key_t;


/// Error code of key
enum key_err
{
    KS_SUCCESS,      // no error
    KS_ERR_EXCEED,   // exceed max count of keys
    KS_ERR_GHOST,    // exist ghost keys
};

typedef struct
{
    uint8_t gcnt;
    uint16_t code[KEY_MULTI_MAX_NUM];
} keys_t;

struct key_env_tag
{
    keys_t curr;
    keys_t last;

    uint8_t multi_key_release_debounce; /*!< 组合键松开后消抖一次 */
    uint8_t voice_key_pressed;

    uint32_t curr_press_time;
    uint32_t last_press_time;
};

void keys_init(void);

void keys_sleep(void);

void keys_proc(void);

bool rc_is_power_key(void);

void keys_proc_ble_wakeup(void);

bool keys_active(void);

#endif  //_KEYS_H_
