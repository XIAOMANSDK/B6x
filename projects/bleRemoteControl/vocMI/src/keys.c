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
#include "prf_hids.h"
#include "keys.h"
#include "app_user.h"
#include "gapc_api.h"
#include "pt_priv_data.h"

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

/// Macro for ghost-key detection (0-disable, 1-enable)
#define KEY_GHOST             (1)

/// Macro for key debounce (0-disable, else (1 << n) - 1)
#define KEY_DEBOUNCE          (0x07)

// 组合键函数定义
static void pt_keys_enter_pair_mode_cb(void)
{
    PT_LOGD("type:%d state:%d\n", pt_get_adv_type(), app_state_get());
    if (pt_get_adv_type() == ADV_REPAIR)
        return;

    deletePairInfo();
    // 连接过先断连，到 BLE_DISCONNECTED 再重启广播；没有连接直接开启广播
    pt_set_adv_type(ADV_REPAIR);
    if (app_state_get() >= APP_CONNECTED)
    {
        gapc_disconnect(app_env.curidx);
    }
    else
    {
        app_adv_action(ACTV_RELOAD);
    }
}

// 按键表定义
#if (KEY_HY)
const uint8_t Key_Row[KEY_ROW_NB] = { KEY_R0, KEY_R1, KEY_R2, KEY_R3 };
const uint8_t Key_Col[KEY_COL_NB] = { KEY_C0, KEY_C1, KEY_C2, KEY_C3 };
const uint8_t Key_Map[KEY_ROW_NB][KEY_COL_NB] =
{
    {PT_KEY_UP,    PT_KEY_RIGHT, PT_KEY_MENU,  PT_KEY_DOWN  },
    {PT_KEY_VOICE,   PT_KEY_ENTER, PT_KEY_BACK,  PT_KEY_VOLUP},
    {PT_KEY_POWER, PT_KEY_LEFT,  PT_KEY_HOME, PT_KEY_VOLDN},
    {KEY_F6,    KEY_F16,   KEY_MUTE, KEY_ESC },
};
#else
const uint8_t Key_Row[KEY_ROW_NB] = { KEY_R0, KEY_R1, KEY_R2 };
const uint8_t Key_Col[KEY_COL_NB] = { KEY_C0, KEY_C1, KEY_C2, KEY_C3 };
const uint8_t Key_Map[KEY_ROW_NB][KEY_COL_NB] = {
    {PT_KEY_ENTER,  PT_KEY_BACK,    PT_KEY_UP,      PT_KEY_DOWN},
    {PT_KEY_VOICE,  PT_KEY_MENU,    PT_KEY_POWER,   PT_KEY_RIGHT},
    {PT_KEY_HOME,   PT_KEY_VOLDN,   PT_KEY_LEFT,    PT_KEY_VOLUP},
};
const pt_double_key_t multi_key_table[] = {
    {{PT_KEY_HOME, PT_KEY_MENU}, "pair mode", pt_keys_enter_pair_mode_cb},
};
#endif

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

static void rows_init(void)
{
    // Row GPIOs as HiZ(no Output no Input)
    GPIO_DIR_CLR(KEY_ROW_MSK);
    GPIO_DAT_CLR(KEY_ROW_MSK);
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

static uint32_t pt_get_curr_time_ms(void)
{
    rtc_time_t time;
    
    time.sec = APBMISC->RTC_SEC_SHD;
    time.ms  = APBMISC->RTC_MS_SHD;
    
    if (time.sec != APBMISC->RTC_SEC_SHD)
    {
        // just past to next second
        time.sec = APBMISC->RTC_SEC_SHD;
        time.ms  = APBMISC->RTC_MS_SHD;
    }
    
    return (time.sec * 1000 + time.ms);
}

static bool pt_key_changed(void)
{
    return (memcmp(&key_env.curr, &key_env.last, sizeof(keys_t)) != 0);
}

static int pt_double_key_handler(uint16_t k1, uint16_t k2)
{
    for (int i = 0; i < PT_ARRAY_SIZE(multi_key_table); i++)
    {
        if ((k1 == multi_key_table[i].keycode[0] && k2 == multi_key_table[i].keycode[1])
        || (k1 == multi_key_table[i].keycode[1] && k2 == multi_key_table[i].keycode[0]))
        {
            PT_LOGD("[double_key] %s(0x%02X 0x%02X)\n", multi_key_table[i].info, k1, k2);
            if (multi_key_table[i].proc_cb)
                multi_key_table[i].proc_cb();
			return 0;
        }
    }

    return -1;
}

void keys_init(void)
{
    rtc_conf(true);

    rows_init();
    cols_init();
    
    #if (KEY_GHOST || KEY_DEBOUNCE)
    memset(&key_buf, 0, sizeof(struct key_buf_tag));
    #endif

    memset(&key_env, 0, sizeof(struct key_env_tag));

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

bool key_press = false;

// 原厂的基础按键扫描实现
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
        GPIO_DAT_SET(1UL << Key_Row[r]);
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
            // press = (key_buf.shake[r][c] & KEY_DEBOUNCE) == KEY_DEBOUNCE ? 1 : 0;
            #else
            press = gpio_get(Key_Col[c]);
            #endif
            
            if (press && Key_Map[r][c])
            {
                // DEBUG("%X(r:%d,c:%d)", kcode, r, c);
                if (keys->gcnt >= PT_MULTI_KEY_MAX_NUM)
                {
                    DEBUG("exceed at(r:%d,c:%d)", r, c);
                    status = KS_ERR_EXCEED;
                    break;
                }

                keys->code[keys->gcnt] = kcode; // Generic Keys
                keys->gcnt++;

                #if (KEY_GHOST)
                key_buf.rowCnt[r]++; // inc row keys
                #endif

            #if (KEY_GHOST || KEY_DEBOUNCE)
                key_buf.colSta[c] |= (1 << r);  // press state
            }
            else
            {
                // key_buf.shake[r][c] = 0;
                key_buf.colSta[c] &= ~(1 << r); // release state
            #endif
            }
        }

        // disable row Output
        GPIO_DAT_CLR(1UL << Key_Row[r]);
        GPIO_DIR_CLR(1UL << Key_Row[r]);
        iom_ctrl(Key_Row[r], IOM_HIZ);

        if (c < KEY_COL_NB) break; // Over error
    }

    // 3 - Detect ghost key
    #if (KEY_GHOST)
    if ((keys->gcnt >= 4) && (status == KS_SUCCESS))
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

    return status;
}

/**
 * @brief 按键变化过滤 + 组合键判断处理
 * 
 * @return uint8_t keycode
 */
static uint8_t pt_keys_scan(void)
{
    uint8_t status = keys_scan();
    if (status != KS_SUCCESS)
        return PT_KEY_INVALID;

    uint8_t keycode = PT_KEY_INVALID;

    if (key_env.curr.gcnt)
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
        }
        #endif
    }

    // 按键判断处理
    bool key_changed = pt_key_changed();
    app_conn_param_update(key_changed);
    if (key_changed)
    {
        // 组合键松开消抖，后续有三键可能需要注意一下
        if(key_env.multi_key_release_debounce && key_env.curr.gcnt == 1)
        {
            key_env.multi_key_release_debounce = 0;
            return PT_KEY_INVALID;
        }
        // PT_LOGD("press=%d, cnt=%d, keycode: %02X %02X %02X\n", key_press, key_env.curr.gcnt, 
        //         key_env.curr.code[0], key_env.curr.code[1], key_env.curr.code[2]);
        // 更新时间戳
        key_env.curr_press_time = pt_get_curr_time_ms();
        key_env.last_press_time = key_env.curr_press_time;
        // 单按键
        if (key_env.curr.gcnt <= 1)
        {
            keycode = key_env.curr.code[0];
        }
    }
    else /*!< 没变化 */
    {
        if (key_press)
        {
            key_env.curr_press_time = pt_get_curr_time_ms();
            if (key_env.curr_press_time - key_env.last_press_time > PT_KEY_LONG_PRESS_TIME_MS)
            {
                // PT_LOGD("longpress: %X %X %X\n", key_env.curr.code[0], key_env.curr.code[1], key_env.curr.code[2]);
                key_env.last_press_time = key_env.curr_press_time;
                key_env.multi_key_release_debounce = 1;
                if (key_env.curr.gcnt == 2)
                {
                    pt_double_key_handler(key_env.curr.code[0], key_env.curr.code[1]);
                }
            }
        }
    }

    return keycode;
}

static void pt_keys_proc(uint8_t keycode)
{
    if (app_state_get() >= APP_BONDED)
    {
#if (VOICE)
        if (keycode == PT_KEY_VOICE)
        {
            micInit();
            pt_hid_voice_request(true);
            key_env.voice_key_pressed = 1;
        }
        else if (keycode == PT_NOKEY && key_env.voice_key_pressed)
        {
            pt_hid_voice_request(false);
            micDeinit();
            key_env.voice_key_pressed = 0;
        }
#endif

        if (keycode == PT_KEY_POWER)
        {
            app_env.user_shutdown_flag = 1;
        }

        pt_hid_kb_report_send(keycode);
    }

    // 如果直连广播 且 配对过，power广播需要打断
    if (app_state_get() < APP_CONNECTED)
    {
        if ((keycode == PT_KEY_POWER) && (pt_get_adv_type() == ADV_DIRECT) && (pt_rcu_is_paired()))
        {
            pt_set_adv_type(ADV_POWER);
            app_adv_action(ACTV_RELOAD);
        }
    }
}

void keys_proc(void)
{
    uint8_t keycode = pt_keys_scan();
    if (keycode == PT_KEY_INVALID)
        return;

    PT_LOGD("keycode=0x%02X\n", keycode);

    pt_keys_proc(keycode);
}

// 重复扫描几次让消抖正常执行
static uint8_t pt_keys_wakeup_detect(void)
{
    for (int i = 0; i < 7; i++)
    {
        uint8_t keycode = pt_keys_scan();
        if((keycode != PT_NOKEY) && (keycode != PT_KEY_INVALID))
        {
            return keycode;
        }
    }

    return PT_NOKEY;
}

void pt_keys_proc_ble_wakeup(void)
{
    uint8_t keycode = pt_keys_wakeup_detect();
    if (keycode == PT_NOKEY)
        return;

    pt_keys_proc(keycode);
}

bool pt_is_power_key(void)
{
    // 检测几次防止刚上电没扫描到
    for (int i = 0; i < 3; i++)
    {
        if((pt_keys_scan() == PT_KEY_POWER))
        {
            // PT_LOGD("power key:0x%02X\n", key_env.curr.code[2]);
            return true;
        }
    }

    return false;
}

bool keys_active(void)
{
    return (key_press || key_env.voice_key_pressed);
}
