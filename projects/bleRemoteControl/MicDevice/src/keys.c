/**
 ****************************************************************************************
 *
 * @file keys.c
 *
 * @brief keys operation.
 *
 ****************************************************************************************
 */
#include "drvs.h"
#include "app.h"
#include "hid_desc.h"
#include "keys.h"
#include "app_user.h"
#include "gapc_api.h"

#if (CFG_SFT_TMR)
#include "sftmr.h"
#endif

#if (DBG_KEYS)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif


/*
 * DEFINES
 ****************************************************************************************
 */
#define RPT_IDX_NONE (RPT_IDX_NB)
/// Macro for ghost-key detection (0-disable, 1-enable)
#define KEY_GHOST             (1)

/// Macro for key debounce (0-disable, else (1 << n) - 1)
#define KEY_DEBOUNCE          (0x00)

const uint8_t Key_Row[KEY_ROW_NB] = { KEY_R0, KEY_R1, KEY_R2, KEY_R3 };

const uint8_t Key_Col[KEY_COL_NB] = { KEY_C0, KEY_C1, KEY_C2, KEY_C3 };

const uint8_t Key_Map[KEY_ROW_NB][KEY_COL_NB] =
{
    {KEY_UP,    KEY_RIGHT, KEY_APP,  KEY_DOWN  },
    {KEY_F5,   KEY_ENTER, KEY_LED,  KEY_VOL_UP},
    {KEY_POWER, KEY_LEFT,  KEY_HOME, KEY_VOL_DN},
    {KEY_F6,    KEY_F16,   KEY_MUTE, KEY_ESC },
};

/// Key Buffer for ghost-detect and debounce 
#if (KEY_GHOST || KEY_DEBOUNCE)
struct key_buf_tag
{
    #if (KEY_GHOST)
    uint8_t rowCnt[KEY_ROW_NB];
    #endif
    
    #if (KEY_GHOST || KEY_DEBOUNCE)
    uint8_t colSta[KEY_COL_NB];
    #endif
    
    #if (KEY_DEBOUNCE)
    uint8_t shake[KEY_ROW_NB][KEY_COL_NB];
    #endif
};

__RETENTION struct key_buf_tag key_buf;
#endif

/// Key Enveriment 
__RETENTION struct key_env_tag key_env;

#if (CFG_SFT_TMR)
#define KEYS_HOLD_TIME     _MS(300)

tmr_tk_t tm_keys_local;
__RETENTION volatile uint16_t proc_flag;

enum proc_fields
{
    PF_KEYS_HOLD_BIT     = (1 << 0),
    PF_BATT_LOW_BIT      = (1 << 1),
    PF_BATT_LVL_BIT      = (1 << 2),
    PF_RC32K_CAL_BIT     = (1 << 3),
};

#define PF_GET_BIT(field)  (proc_flag & (PF_##field##_BIT))
#define PF_SET_BIT(field)  (proc_flag |= (PF_##field##_BIT))
#define PF_CLR_BIT(field)  (proc_flag &= (~PF_##field##_BIT))
#endif

/*
 * FUNCTIONS
 ****************************************************************************************
 */
static void rows_init(void)
{
    // Row GPIOs as HiZ(no Output no Input)
    GPIO_DIR_CLR(KEY_ROW_MSK);
    GPIO_DAT_SET(KEY_ROW_MSK);
    for (uint8_t r = 0; r < KEY_ROW_NB; r++)
    {
        iom_ctrl(Key_Row[r], IOM_HIZ);
    }
}

static void cols_init(void)
{
    // Column GPIOs as Input(Ext PullDown)
    GPIO_DIR_CLR(KEY_COL_MSK);
    GPIO_DAT_CLR(KEY_COL_MSK);
    for (uint8_t c = 0; c < KEY_COL_NB; c++)
    {
        iom_ctrl(Key_Col[c], IE_DOWN);
    }
}

static uint8_t keys_info(const keys_t *keys)
{
    if (keys->gcnt == 0)
        return KI_NOKEY;
    
//    DEBUG("keys_info:%d, %d", keys->mcnt, keys->gcnt);
//    debugHex(keys->code, RPT_LEN_KB);

    uint8_t kcode = keys->code[2];
    if (keys->gcnt == 1)
    {
        if (kcode == KEY_HOME)
        {
            return KI_HOTKEY_HOME;
        }
        
        if (kcode == KEY_ESC)
        {
            return KI_HOTKEY_MUTE;
        }
        
        if (kcode == KEY_F5)
        {
            return KI_HOTKEY_VOICE;
        }
        
        if (kcode == KEY_LED)
        {
            return KI_HOTKEY_BACK;
        }
    }
    // HOME+APP
    else if (keys->gcnt == 2)
    {
        if (((keys->code[2] == KEY_HOME) && (keys->code[3] == KEY_APP))
            || ((keys->code[2] == KEY_APP) && (keys->code[3] == KEY_HOME)))
        {
            return KI_HOTKEY_PAIR;
        }
    }

    return KI_GEKEY;
}

static uint8_t keys_encode(const keys_t *keys, uint8_t *report)
{
    uint8_t rep_idx = RPT_IDX_NONE;
    
    memset(report, 0, RPT_LEN_KB);
//    DEBUG("keys_info:%d, %d", keys->mcnt, keys->gcnt);
//    debugHex(keys->code, RPT_LEN_KB);
    switch (keys->info)
    {
        case KI_GEKEY:
        {
            // copy Modifier Keys(windows replace RGUI with RCTRL)
            if ((keys->code[0] & KEY_BIT_RGUI) && (key_env.sys == WINDOWS))
                report[0] = (keys->code[0] | KEY_BIT_RCTRL) & ~KEY_BIT_RGUI;
            else
                report[0] = (keys->code[0]);

            // copy Generic Keys
            memcpy(&report[2], &keys->code[2], keys->gcnt);
            
            rep_idx = RPT_IDX_KB;
        } break;
        
        case KI_HOTKEY_HOME:
        {
            // HOME
            rep_idx = RPT_IDX_MEDIA;
            report[1] = 1 << 6;
        } break;
        
        case KI_HOTKEY_BACK:
        {
            // Back
            rep_idx = RPT_IDX_MEDIA;
            report[1] = 1 << 3;
        } break;
        
        case KI_HOTKEY_MUTE:
        {
            // Mute
            rep_idx = RPT_IDX_MEDIA;
            report[1] = 1 << 0;
        } break;
        
        case KI_HOTKEY_VOICE:
        {
            // Mute
            // copy Generic Keys
            memcpy(&report[2], &keys->code[2], keys->gcnt);
            rep_idx = RPT_IDX_KB;
            
            #if (VOICE)
            micInit();
            #endif
        } break;
        
        case KI_NOKEY:
        case KI_ERROR:
        case KI_HOTKEY_PAIR:
        default:
        {
//            rep_idx = RPT_IDX_NONE;
        } break;
    }
    
    DEBUG("info:%d, rep_idx:%d", keys->info, rep_idx);
    debugHex(report, RPT_LEN_KB);
    return rep_idx;
}

static bool keys_send(uint8_t repidx, const uint8_t *report)
{
    uint8_t replen;

    if (repidx == RPT_IDX_MEDIA)
    {
        replen = RPT_LEN_MEDIA;
    }
    #if (HID_RPT_SYSTEM)
    else if (repidx == RPT_IDX_SYSTEM)
    {
        replen = RPT_LEN_SYSTEM;
    }
    #endif
    else
    {
        replen = RPT_LEN_KB;
    }
    
    DEBUG("keys_send(%d,%d)", repidx, replen);
    return !hids_report_send(app_env.curidx, repidx, replen, report);
}

/*
 * EXPORT FUNCTIONS
 ****************************************************************************************
 */

void keys_init(void)
{
    rows_init();
    cols_init();
    
    #if (KEY_GHOST || KEY_DEBOUNCE)
    memset(&key_buf, 0, sizeof(struct key_buf_tag));
    #endif

    memset(&key_env, 0, sizeof(struct key_env_tag));
    key_env.rep_idx = RPT_IDX_NONE;
    // no first report
    key_env.repok = 1;
    
    #if (CFG_SFT_TMR)
    proc_flag = 0;
    #endif
}

void keys_sleep(void)
{
    DEBUG("Poweroff");
    GPIO_DIR_SET_LO(KEY_COL_MSK);
    wakeup_io_sw(KEY_ROW_MSK, KEY_ROW_MSK);
    core_pwroff(CFG_WKUP_IO_EN | WKUP_IO_LATCH_N_BIT);
}

void keys_conf(uint8_t os)
{
    key_env.sys = os;
}

bool key_press = false;


static uint8_t keys_scan(void)
{
    uint8_t r, c, colSta;
    uint8_t status = KS_SUCCESS;
    keys_t *keys = &key_env.curr;
    
    // 0 - All Row GPIOs as HiZ
    rows_init();
    
    // 1 - Init keys env, first backup
    key_env.last = *keys;
    memset(keys, 0, sizeof(keys_t));
    
    // 2 - Scan row by row
    for (r = 0; r < KEY_ROW_NB; r++)
    {
        // Enable row Output(High)
        GPIO_DIR_SET(1UL << Key_Row[r]); 
        
        // Init keys buf
        #if (KEY_GHOST)
        key_buf.rowCnt[r] = 0;
        #endif
        
        // Judge col by col
        for (c = 0; c < KEY_COL_NB; c++)
        {
            uint8_t press = 0;
            uint8_t kcode = Key_Map[r][c];
            
            if (kcode == 0) continue; // empty code
            
            #if (KEY_DEBOUNCE)
            // Shift right to update
            key_buf.shake[r][c] <<= 1;
            
            if (gpio_get(Key_Col[c]))
            {
                key_buf.shake[r][c] |= 1;
            }
            
            // Judge real state
            press = ((key_buf.shake[r][c] & KEY_DEBOUNCE) == KEY_DEBOUNCE)
                    || (((key_buf.shake[r][c] & KEY_DEBOUNCE) != 0) && (key_buf.colSta[c] & (1 << r)));
            #else
            press = gpio_get(Key_Col[c]);
            #endif
            
            if (press && Key_Map[r][c])
            {
//                DEBUG("%X(r:%d,c:%d)", kcode, r, c);
   
                if (keys->gcnt >= 6)
                {
                    DEBUG("exceed at(r:%d,c:%d)", r, c);
                    status = KS_ERR_EXCEED;
                    break;
                }
                
                keys->code[2+keys->gcnt] = kcode; // Generic Keys
                keys->gcnt++;
                
                #if (KEY_GHOST)
                key_buf.rowCnt[r]++; // inc row keys
                #endif
                
            #if (KEY_GHOST || KEY_DEBOUNCE)
                key_buf.colSta[c] |= (1 << r);  // press state
            }
            else
            {
                key_buf.colSta[c] &= ~(1 << r); // release state
            #endif
            }
        }
        
        // disable row Output
        GPIO_DIR_CLR(1UL << Key_Row[r]);
        
        if (c < KEY_COL_NB) break; // Over error
    }

    if (keys->gcnt)
    {
        key_press = true;
    }
    else
    {
        key_press = false;
        
        #if (VOICE)
        if (SADC->CTRL.SADC_DMAC_EN)
        {
            SADC->CTRL.SADC_DMAC_EN = 0;  // voice stop
            DEBUG("Voice:Send-%d OK-%d", voiceSendNB, voiceSendOK);        
        }        
        #endif
    }
    
    // 3 - Detect ghost key
    #if (KEY_GHOST)
    if ((keys->mcnt + keys->gcnt >= 4) && (status == KS_SUCCESS))
    {
        // ghost in 'L' style(3+1) or not
        for (r = 0; r < KEY_ROW_NB; r++)
        {
            if (key_buf.rowCnt[r] >= 2)
            {
                uint8_t rowBit = (1 << r);
                for (c = 0; c < KEY_COL_NB; c++)
                {
                    colSta = key_buf.colSta[c];
                    if ((colSta & (1 << r))/* && (ONE_BITS(key_buf.colSta[c]) >= 2)*/)
                    {
                        if ((rowBit & colSta) != (1 << r))
                        {
                            DEBUG("ghost at(r:%d,c:%d,r:0x%x)", r, c, key_buf.colSta[c]);
                            status = KS_ERR_GHOST;
                            break;
                        }
                        else
                        {
                            rowBit |= colSta;
                        }
                    }
                }
            }
        }
    }
    #endif
    
    for (uint8_t r = 0; r < KEY_ROW_NB; r++)
    {
        iom_ctrl(Key_Row[r], IOM_PULLDOWN);
    }
    
    // 4 - Judge key info
    keys->info = (status == KS_SUCCESS) ? keys_info(keys) : KI_ERROR;

    return keys->info;
}

static bool keys_changed(void)
{
//    DEBUG("cinfo:%d, linfo:%d", key_env.curr.info,key_env.last.info);
    if ((key_env.curr.info != key_env.last.info)
        || ((key_env.curr.info <= KI_FNKEY) && (memcmp(key_env.curr.code, key_env.last.code, RPT_LEN_KB) != 0)))
    {
        return true;
    }
    else
    {
        return false;
    }
}
bool key_change;


static void keys_report(void)
{
    uint8_t rep_idx, report[RPT_LEN_KB];
    
    key_change = keys_changed();
//    DEBUG("key_change:%d", key_change);
    app_conn_param_update(key_change);
    
    // 0 - ignore same with last(send ok)
    if (key_env.repok && !key_change)
    {
        return;
    }
    
    rep_idx = keys_encode(&key_env.curr, report);
    
    if (rep_idx == key_env.rep_idx)
    {
        // 1 - report same, only send curr pressed keys
        if ((rep_idx != RPT_IDX_NONE) || (!key_env.repok))
        {
            key_env.repok = keys_send(rep_idx, report);
        }
    }
    else 
    {
        // 2 - report diff, send curr or release last pressed keys
        if (key_env.rep_idx == RPT_IDX_NONE)
        {
            key_env.repok = keys_send(rep_idx, report);
        }
        else 
        {
            if (key_env.curr.info != KI_ERROR)
            {
                rep_idx = RPT_IDX_NONE; // release last
                key_env.repok = keys_send(key_env.rep_idx, NULL);
            }
        }
    }

    // 3 - update rep_idx when ok
    if (key_env.repok)
    {
        if (key_env.curr.info != KI_ERROR)
        {
            key_env.rep_idx = rep_idx;
        }
    }
}

/// Keys Procedure
static void hotkey_local(uint8_t info)
{
    DEBUG("hotkey_local:%d, app_sta:%d", info, app_state_get());
    switch (info)
    {
        // 左右同时按下开始进配对模式
        case KI_HOTKEY_PAIR:
        {
            g_no_action_cnt = 0;
            
            DEBUG("PAIR MODE");
            deletePairInfo();
            if (app_state_get() >= APP_CONNECTED)
            {
                gapc_disconnect(app_env.curidx);
            }
        } break;
        
        default:
        {
        } break;
    }
}

void keys_proc(void)
{
    uint8_t info = keys_scan();
//    DEBUG("info:%d   :%d", info, app_state_get());
    if ((info >= KI_HOTKEY_LOCAL) && (info < KI_HOTKEY_HOST))
    {
        // hotkey_local proc
        if (keys_changed())
        {
            hotkey_local(info);
        }
    }
    else if (app_state_get() >= APP_CONNECTED)
    {
        keys_report();
    }
}
