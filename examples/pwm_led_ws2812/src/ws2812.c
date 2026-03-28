/**
 ****************************************************************************************
 *
 * @file ws2812.c
 ****************************************************************************************
 */

#include "drvs.h"
#include "sftmr.h"
#include "ws2812.h"

/*
 * DEFINES
 ****************************************************************************************
 */
// DMA Request Cycle Too Long.
#if (SYS_CLK < 2)
    #error "!!!system clock too low.!!!"
#endif

#ifndef PA_WS2812
    #define PA_WS2812 8
#endif

// LED
#define LED_NUM     (4) // 固定LED数量

#define PWM_TMR_PSC (0)
#define CLK_FREQ    (SYS_CLK + 1)
#define ARR_CNT     (20 * CLK_FREQ)

// freq 1.25us
#define PWM_TMR_ARR (ARR_CNT - 1)
#define ZERO_PULSE  (5 * CLK_FREQ)  // 占空比 ZERO_PULSE
#define ONE_PULSE   (15 * CLK_FREQ) // 占空比 ONE_PULSE

// DMA
#define DMA_PWM_LED_CH    (DMA_CH0)
#define PWM_DMA_INT       (0x01UL << DMA_PWM_LED_CH)
#define CFG_PWM_CCER_SIPH (PWM_CCER_SIPH | PWM_CCxDE_BIT)

// 一个灯总共24bits, 每个bit一个脉冲控制.
#define RGB_LED_DATA_LEN 24
#define G_LED_POS        0
#define R_LED_POS        8
#define B_LED_POS        16
// 前后字节为0
#define PWM_BUF_LEN      (1 + (LED_NUM * RGB_LED_DATA_LEN) + 1)

__DATA_ALIGNED(4) uint8_t pwm_buffer[PWM_BUF_LEN]; // PWM缓冲区（24bit/LED）

#define DMA_CTMR_CH1_SEND()                                                                        \
    dma_chnl_conf(DMA_PWM_LED_CH, (uint32_t)&pwm_buffer[PWM_BUF_LEN - 1], DMA_PTR_CTMR_CH1,        \
                  TRANS_PER_WR(CCM_BASIC, PWM_BUF_LEN, IN_BYTE, IN_BYTE))

// 0~255 LED亮度
#define LED_LIGHT_DEFAULT   (31)
#define LED_LIGHT_R         (LED_LIGHT_DEFAULT) // LED红色亮度
#define LED_LIGHT_G         (LED_LIGHT_DEFAULT) // LED绿色亮度
#define LED_LIGHT_B         (LED_LIGHT_DEFAULT) // LED蓝色亮度

// LED COLOR
// No Light
#define LED_COLOR_N 0
// Blue
#define LED_COLOR_B 1
// Red
#define LED_COLOR_R 2
// Green
#define LED_COLOR_G 4
// Yellow
#define LED_COLOR_Y (LED_COLOR_R | LED_COLOR_G)
// Cyan
#define LED_COLOR_C (LED_COLOR_B | LED_COLOR_G)
// Violet
#define LED_COLOR_V (LED_COLOR_B | LED_COLOR_R)

#define COLOR(_c)   (LED_COLOR_##_c)

#define LED_SCAN_PERIOD _MS(10)

struct ws2812_env_tag
{
    uint16_t next_time; // sftmr_tick
    uint8_t repeat  : 1;
    uint8_t led_mode: 7;
    uint8_t item_idx;
};

struct led_item
{
    uint16_t led_time;       // sftmr_tick

    uint16_t led0_color : 4; // LED0颜色
    uint16_t led1_color : 4; // LED1颜色
    uint16_t led2_color : 4; // LED2颜色
    uint16_t led3_color : 4; // LED3颜色
};

const struct led_item LED_IDLE_LIST[] =
{
    // led_time, led0_color, led1_color, led2_color, led3_color
    {_MS(200), COLOR(C), COLOR(N), COLOR(N), COLOR(N)},
    {_MS(200), COLOR(N), COLOR(G), COLOR(N), COLOR(N)},
    {_MS(200), COLOR(N), COLOR(N), COLOR(R), COLOR(N)},
    {_MS(200), COLOR(N), COLOR(N), COLOR(N), COLOR(B)},
    {_MS(200), COLOR(N), COLOR(N), COLOR(R), COLOR(N)},
    {_MS(200), COLOR(N), COLOR(G), COLOR(N), COLOR(N)},
};

const struct led_item LED_FAST_LIST[] =
{
    // led_time, led0_color, led1_color, led2_color, led3_color
    {_MS(200), COLOR(B), COLOR(B), COLOR(B), COLOR(B)},
    {_MS(200), COLOR(N), COLOR(N), COLOR(N), COLOR(N)},
};

const struct led_item LED_SLOW_LIST[] =
{
    // led_time, led0_color, led1_color, led2_color, led3_color
    {_MS(1000), COLOR(G), COLOR(G), COLOR(G), COLOR(G)},
    {_MS(1000), COLOR(N), COLOR(N), COLOR(N), COLOR(N)},
};


const struct led_item LED_CHARGING_LIST[] =
{
    // led_time, led0_color, led1_color, led2_color, led3_color
    {_MS(500), COLOR(N), COLOR(N), COLOR(N), COLOR(N)},
    {_MS(500), COLOR(R), COLOR(N), COLOR(N), COLOR(N)},
    {_MS(500), COLOR(R), COLOR(R), COLOR(N), COLOR(N)},
    {_MS(500), COLOR(R), COLOR(R), COLOR(R), COLOR(N)},
    {_MS(500), COLOR(R), COLOR(R), COLOR(R), COLOR(R)},
    {_MS(150), COLOR(N), COLOR(N), COLOR(N), COLOR(N)},
    {_MS(150), COLOR(R), COLOR(R), COLOR(R), COLOR(R)},
    {_MS(150), COLOR(N), COLOR(N), COLOR(N), COLOR(N)},
    {_MS(150), COLOR(R), COLOR(R), COLOR(R), COLOR(R)},
};

const struct led_item LED_HOLD_LIST[] =
{
    // led_time, led0_color, led1_color, led2_color, led3_color
    {_MS(0), COLOR(R), COLOR(R), COLOR(R), COLOR(R)},
};

struct led_table
{
    const struct led_item *list;
    uint8_t cnt;
    uint8_t repeat;
};

#define LED_TABLE_ADD(rep, _mode) \
    [LED_MODE_##_mode] = { .list = LED_##_mode##_LIST, .cnt = sizeof(LED_##_mode##_LIST) / sizeof(struct led_item), .repeat = rep }

const struct led_table led_tables[] =
{
    LED_TABLE_ADD(1, IDLE),
    LED_TABLE_ADD(1, FAST),
    LED_TABLE_ADD(1, SLOW),
    LED_TABLE_ADD(1, CHARGING),
    LED_TABLE_ADD(0, HOLD),
};

__RETENTION volatile struct ws2812_env_tag ws2812_env;

/*
 * FUNCTIONS
 ****************************************************************************************
 */

// 将LED颜色数组转换为PWM波形数据（24-bit, GRB）
void ws2812_update_index(uint8_t led_idx, uint8_t b, uint8_t r, uint8_t g)
{
    uint8_t *p_base = pwm_buffer + 1 + (led_idx * RGB_LED_DATA_LEN);
    uint8_t *pG = p_base + G_LED_POS;
    uint8_t *pR = p_base + R_LED_POS;
    uint8_t *pB = p_base + B_LED_POS;

    uint8_t mask = 0x80;

    while (mask)
    {
        *pG++ = (g & mask) ? ONE_PULSE : ZERO_PULSE;
        *pR++ = (r & mask) ? ONE_PULSE : ZERO_PULSE;
        *pB++ = (b & mask) ? ONE_PULSE : ZERO_PULSE;
        mask >>= 1;
    }
}

void ws2812_update(void)
{
    const struct led_item *item = &led_tables[ws2812_env.led_mode].list[ws2812_env.item_idx];
    uint8_t colors[LED_NUM];

    colors[0] = item->led0_color;
    colors[1] = item->led1_color;
    colors[2] = item->led2_color;
    colors[3] = item->led3_color;

    for (uint8_t i = 0; i < LED_NUM; i++)
    {
        uint8_t col = colors[i];
        uint8_t b = col & LED_COLOR_B;
        uint8_t r = col & LED_COLOR_R;
        uint8_t g = col & LED_COLOR_G;

        ws2812_update_index(i, b ? LED_LIGHT_B : 0, r ? LED_LIGHT_R : 0, g ? LED_LIGHT_G : 0);
    }
}

void ws2812_play(uint8_t mode)
{
    if (mode >= LED_MODE_MAX)
    {
        return;
    }

    if (DMACHNL_INT_GET(DMA_PWM_LED_CH))
    {
        DMACHNL_INT_CLR(DMA_PWM_LED_CH);
    }
    const struct led_table *led_table = &led_tables[mode];

    ws2812_env.led_mode  = mode;
    ws2812_env.item_idx  = 0;
    ws2812_env.repeat    = led_table->repeat;

    ws2812_env.next_time = sftmr_tick() + led_table->list[0].led_time;

    ws2812_update();
    DMA_CTMR_CH1_SEND();
}

/// Tick timeout arrived
#define TMR_TICK_OUT(now, out) ((tmr_tk_t)((now) - (out)) <= SFTMR_DELAY_MAX)

static tmr_tk_t sftmr_ws2812_scan(tmr_id_t id)
{
    (void)id;
    if (TMR_TICK_OUT(sftmr_tick(), ws2812_env.next_time) && ws2812_env.repeat)
    {
        if (DMACHNL_INT_GET(DMA_PWM_LED_CH))
        {
            DMACHNL_INT_CLR(DMA_PWM_LED_CH);
        }

        const struct led_table *led_table = &led_tables[ws2812_env.led_mode];

        ws2812_env.item_idx  = (ws2812_env.item_idx + 1) % led_table->cnt;
        ws2812_env.next_time = sftmr_tick() + led_table->list[ws2812_env.item_idx].led_time;

        ws2812_update();
        DMA_CTMR_CH1_SEND();
    }

    return (LED_SCAN_PERIOD);
}

// DMA INIT PWM INIT
void ws2812_init(void)
{
    dma_init();
    memset((uint8_t *)&pwm_buffer, 0, sizeof(pwm_buffer));
    memset((uint8_t *)&ws2812_env, 0x00, sizeof(ws2812_env));

    iocsc_ctmr_chnl(PA_WS2812, PA_MAX);
    pwm_init(PWM_CTMR, PWM_TMR_PSC, PWM_TMR_ARR);

    pwm_chnl_cfg_t chnl_conf;
    chnl_conf.ccmr = PWM_CCMR_MODE1;
    chnl_conf.duty = 0;
    chnl_conf.ccer = CFG_PWM_CCER_SIPH;
    pwm_chnl_set(PWM_CTMR_CH1, &chnl_conf); // 对应IO的pwm通道
    pwm_start(PWM_CTMR);

    DMA_CTMR_CHx_INIT(DMA_PWM_LED_CH, 1);

    sftmr_start(LED_SCAN_PERIOD, sftmr_ws2812_scan);
}
