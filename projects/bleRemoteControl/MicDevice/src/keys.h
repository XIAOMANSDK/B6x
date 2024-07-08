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


typedef struct key_sca
{
    uint8_t info;    // @see enum key_inf
    uint8_t mcnt;    // count of mk(@see enum key_bit)
    uint8_t gcnt;    // count of gk(@see enum generic_key)
    uint8_t code[8]; // 0:Modifier Keys, 1:Fn Keys, 2~7:Generic Keys
} keys_t;

struct key_env_tag
{
    uint8_t sys;
    
    keys_t  curr;    // current scanned keys
    keys_t  last;    // last scanned keys, backup
    uint8_t rep_idx;
    uint8_t repok;
};

/// Error code of key
enum key_err
{
    KS_SUCCESS,      // no error
    KS_ERR_EXCEED,   // exceed max count of keys
    KS_ERR_GHOST,    // exist ghost keys
};

/// Information of key
enum key_inf
{
    KI_NOKEY,        // release (no key)
    KI_ERROR,        // error(key_err)
    KI_GEKEY,        // generic(normal key)
    KI_FNKEY,        // Fn press(without hot-key)
    
    // Hot Keys Group for local device, customize more...
    KI_HOTKEY_LOCAL, // Local Base as split line
    KI_HOTKEY_PAIR,  // PAIR MODE, 

    
    // Hot Keys Group for host report, customize more...
    KI_HOTKEY_HOST,  // Host Base as split line
    KI_HOTKEY_HOME,
    KI_HOTKEY_BACK,
    KI_HOTKEY_MUTE,
    KI_HOTKEY_VOICE, // VOICE MODE, 
};

void keys_init(void);

void keys_sleep(void);

void keys_conf(uint8_t os);

void keys_proc(void);
#endif  //_KEYS_H_
