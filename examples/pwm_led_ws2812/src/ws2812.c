/**
 ****************************************************************************************
 *
 * @file ws2812.c
 *
 * @brief WS2812 RGB LED driver using PWM+DMA.
 *
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

// Fixed LED count
#define LED_NUM     (4)

#define PWM_TMR_PSC (0)
#define CLK_FREQ    (SYS_CLK + 1)
#define ARR_CNT     (20 * CLK_FREQ)

// 1.25us period
#define PWM_TMR_ARR (ARR_CNT - 1)
#define ZERO_PULSE  (5 * CLK_FREQ)   // Duty cycle for logic ZERO
#define ONE_PULSE   (15 * CLK_FREQ)  // Duty cycle for logic ONE

// DMA
#define DMA_PWM_LED_CH    (DMA_CH0)
#define PWM_DMA_INT       (0x01UL << DMA_PWM_LED_CH)
#define CFG_PWM_CCER_SIPH (PWM_CCER_SIPH | PWM_CCxDE_BIT)

// 24 bits per LED, one pulse per bit
#define RGB_LED_DATA_LEN 24
#define G_LED_POS        0
#define R_LED_POS        8
#define B_LED_POS        16

// Leading/trailing bytes are zero (reset code)
#define PWM_BUF_LEN      (1 + (LED_NUM * RGB_LED_DATA_LEN) + 1)

// Tick timeout check (mirrors sftmr.c internal macro, not exported via sftmr.h)
#define WS2812_TICK_OUT(now, out) ((tmr_tk_t)((now) - (out)) <= SFTMR_DELAY_MAX)

__DATA_ALIGNED(4) static uint8_t pwm_buffer[PWM_BUF_LEN];  // PWM buffer (24-bit per LED)

#define DMA_CTMR_CH1_SEND()                                                                        \
    dma_chnl_conf(DMA_PWM_LED_CH, (uint32_t)&pwm_buffer[PWM_BUF_LEN - 1], DMA_PTR_CTMR_CH1,        \
                  TRANS_PER_WR(CCM_BASIC, PWM_BUF_LEN, IN_BYTE, IN_BYTE))

// LED brightness (0~255)
#define LED_LIGHT_DEFAULT   (31)
#define LED_LIGHT_R         (LED_LIGHT_DEFAULT)  // Red brightness
#define LED_LIGHT_G         (LED_LIGHT_DEFAULT)  // Green brightness
#define LED_LIGHT_B         (LED_LIGHT_DEFAULT)  // Blue brightness

// LED COLOR
#define LED_COLOR_N 0  // No Light
#define LED_COLOR_B 1  // Blue
#define LED_COLOR_R 2  // Red
#define LED_COLOR_G 4  // Green
#define LED_COLOR_Y (LED_COLOR_R | LED_COLOR_G)  // Yellow
#define LED_COLOR_C (LED_COLOR_B | LED_COLOR_G)  // Cyan
#define LED_COLOR_V (LED_COLOR_B | LED_COLOR_R)  // Violet

#define COLOR(_c)   (LED_COLOR_##_c)

#define LED_SCAN_PERIOD _MS(10)

struct ws2812_env_tag
{
    uint16_t next_time;
    uint8_t repeat  : 1;
    uint8_t led_mode: 7;
    uint8_t item_idx;
};

struct led_item
{
    uint16_t led_time;

    uint16_t led0_color : 4;
    uint16_t led1_color : 4;
    uint16_t led2_color : 4;
    uint16_t led3_color : 4;
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

static const struct led_table led_tables[] =
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

/**
 ****************************************************************************************
 * @brief Convert LED color to PWM waveform data for a single LED (24-bit, GRB).
 *
 * @param[in] led_idx  LED index (0 ~ LED_NUM-1)
 * @param[in] b        Blue component (0~255)
 * @param[in] r        Red component (0~255)
 * @param[in] g        Green component (0~255)
 ****************************************************************************************
 */
static void ws2812_update_index(uint8_t led_idx, uint8_t b, uint8_t r, uint8_t g)
{
    if (led_idx >= LED_NUM)
    {
        return;
    }

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

/**
 ****************************************************************************************
 * @brief Update all LEDs from the current animation item.
 ****************************************************************************************
 */
static void ws2812_update(void)
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

/**
 ****************************************************************************************
 * @brief Start playing an LED animation mode.
 *
 * @param[in] mode  Animation mode index (see led_mode_t)
 ****************************************************************************************
 */
void ws2812_play(uint8_t mode)
{
    if (mode >= LED_MODE_MAX)
    {
        return;
    }

    const struct led_table *led_table = &led_tables[mode];
    if (led_table->cnt == 0)
    {
        return;
    }

    if (DMACHNL_INT_GET(DMA_PWM_LED_CH))
    {
        DMACHNL_INT_CLR(DMA_PWM_LED_CH);
    }

    ws2812_env.led_mode  = mode;
    ws2812_env.item_idx  = 0;
    ws2812_env.repeat    = led_table->repeat;
    ws2812_env.next_time = sftmr_tick() + led_table->list[0].led_time;

    ws2812_update();
    DMA_CTMR_CH1_SEND();
}

/**
 ****************************************************************************************
 * @brief Soft timer callback for LED animation scanning.
 *
 * @param[in] id  Timer ID
 *
 * @return Next scan period in ticks
 ****************************************************************************************
 */
static tmr_tk_t sftmr_ws2812_scan(tmr_id_t id)
{
    (void)id;

    if (WS2812_TICK_OUT(sftmr_tick(), ws2812_env.next_time) && ws2812_env.repeat)
    {
        const struct led_table *led_table = &led_tables[ws2812_env.led_mode];

        if (led_table->cnt == 0)
        {
            return LED_SCAN_PERIOD;
        }

        if (DMACHNL_INT_GET(DMA_PWM_LED_CH))
        {
            DMACHNL_INT_CLR(DMA_PWM_LED_CH);
        }

        ws2812_env.item_idx++;
        if (ws2812_env.item_idx >= led_table->cnt)
        {
            ws2812_env.item_idx = 0;
        }
        ws2812_env.next_time = sftmr_tick() + led_table->list[ws2812_env.item_idx].led_time;

        ws2812_update();
        DMA_CTMR_CH1_SEND();
    }

    return (LED_SCAN_PERIOD);
}

/**
 ****************************************************************************************
 * @brief Initialize WS2812 PWM+DMA driver and start animation scanner.
 ****************************************************************************************
 */
void ws2812_init(void)
{
    dma_init();
    memset(pwm_buffer, 0, sizeof(pwm_buffer));

    ws2812_env.next_time = 0;
    ws2812_env.repeat    = 0;
    ws2812_env.led_mode  = 0;
    ws2812_env.item_idx  = 0;

    iocsc_ctmr_chnl(PA_WS2812, PA_MAX);
    pwm_init(PWM_CTMR, PWM_TMR_PSC, PWM_TMR_ARR);

    pwm_chnl_cfg_t chnl_conf;
    chnl_conf.ccmr = PWM_CCMR_MODE1;
    chnl_conf.duty = 0;
    chnl_conf.ccer = CFG_PWM_CCER_SIPH;
    pwm_chnl_set(PWM_CTMR_CH1, &chnl_conf);
    pwm_start(PWM_CTMR);

    DMA_CTMR_CHx_INIT(DMA_PWM_LED_CH, 1);

    sftmr_start(LED_SCAN_PERIOD, sftmr_ws2812_scan);
}
