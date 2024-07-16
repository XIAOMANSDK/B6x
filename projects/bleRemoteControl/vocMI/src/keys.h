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

#define PT_KEY_LONG_PRESS_TIME_MS   (3000) /*!< 所有组合键长按时间,单位ms */

#define PT_MULTI_KEY_MAX_NUM           (3) /*!< 组合键个数 */

// 按小米国内按键实际布局重命名
enum {
    PT_NOKEY = 0x0000,
    PT_KEY_POWER = KEY_POWER,
    PT_KEY_VOICE = KEY_F5,
    PT_KEY_UP = KEY_UP,
    PT_KEY_DOWN = KEY_DOWN,
    PT_KEY_LEFT = KEY_LEFT,
    PT_KEY_RIGHT = KEY_RIGHT,
    PT_KEY_ENTER = KEY_ENTER,
    PT_KEY_HOME = KEY_HOME,
    PT_KEY_BACK = KEY_LED,
    PT_KEY_MENU = KEY_APP,
    PT_KEY_VOLUP = KEY_VOL_UP,
    PT_KEY_VOLDN = KEY_VOL_DN,

    PT_KEY_INVALID = 0xFF
};

typedef struct
{
    uint16_t keycode[2];
    const char *info;
    void (*proc_cb)(void);
} pt_double_key_t;


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
    uint8_t code[PT_MULTI_KEY_MAX_NUM];
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

bool pt_is_power_key(void);

void pt_keys_proc_ble_wakeup(void);

bool keys_active(void);

#endif  //_KEYS_H_
