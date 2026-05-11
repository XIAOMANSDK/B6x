/**
 ****************************************************************************************
 *
 * @file lcd.c
 *
 * @brief LCD Driver, Support two LCDs in Mirror.
 *       [KAC-1.33' IPS_LCD, Pixel:240*240, Driver IC:ST7789, Interface:SPI] or other
 * 
 ****************************************************************************************
 */

#include "lcd.h"
#include "b6x.h"
#include "drvs.h"
#include "regs.h"


/*
 * DEFINES
 ****************************************************************************************
 */

/// LCD's Driver Configure
// driver IC (**If only one LCD, set DRV_LCD2 as IC_NULL**)
#define DRV_LCD1                (IC_ST7789)
#define DRV_LCD2                (IC_ST7789)//(IC_GC9A01)
// memory data access control(MADCTL, **IC_GC9A01 need MADCTL_RGB_BIT**)
#define MAD_LCD1                (PORTRAIT_FLIP)
#define MAD_LCD2                (PORTRAIT_NORMAL) //|MADCTL_RGB_BIT)
// vertical scrolling start address(VSCRSADD)
#define VSA_LCD1                (80) // 320-240=80 when IC_ST7789 with MADCTL_MY_BIT
#define VSA_LCD2                (0)
// background color
#define BG_COLOR                (RGB565_BLACK)
// ms delay for waiting
#define MS_SLPOUT               (30)
#define MS_DISPON               (120)

/// LCDs' Pads defines
// common Pads
#define PA_LCD_CLK              (11)
#define PA_LCD_SDA              (12)
#define PA_LCD_RST              (13)
// special Pads (**If only a LCD, set PA_LCD_DC2=PA_LCD_DC1 and PA_LCD_CS2=PA_LCD_CS1**)
#define PA_LCD_DC1              (14)
#define PA_LCD_CS1              (15)
#define PA_LCD_DC2              (16)
#define PA_LCD_CS2              (17)
// Backlight ON/OFF (default ON, maybe No Wiring)
#define PA_LCD_BLK              (18)
#define LCD_BLK_ON()            GPIO_DAT_SET(1UL << PA_LCD_BLK)
#define LCD_BLK_OFF()           GPIO_DAT_CLR(1UL << PA_LCD_BLK)

/// DMA for spi tx(mosi)
#define DMA_CH_SPI_TX           (DMA_CH0)
#define DMA_LEN_MAX             (1024) // <=1024

/// Time delay func
#define delay_ms(n)             bootDelayMs(n)

/// Align value on the multiple of 4 equal or nearest higher.
//  @param[in] val Value to align.
#define ALIGN4_HI(_val)         (((_val) + 3) & ~3)
// byte len to word len.
#define ALIGN4_HI_WL(_val)      ((((_val) + 3) & ~3) >> 2)

/// flash data pointer to flash offset
#define PDATA_OFFSET(p_data)    ((uint32_t)(p_data) - FLASH_BASE)

/// LCD environment structure
typedef struct {
    volatile uint8_t  state;

    // curr index
    uint8_t  selId;

    // data info for DMA
    uint16_t irq_cnt;
    uint32_t len_rem;
    uint32_t dat_pos;
} lcd_env_t;


/*
 * GLOBAL VARIABLE AND CONST DATA
 ****************************************************************************************
 */

/// Environment
static lcd_env_t lcd_env;

/// DMA Ping-Pong Buffer
__attribute__((aligned (4)))
static uint8_t dma_buff[2][DMA_LEN_MAX];

/// Initialization Sequence Format: [cmd, param_len, params...]
///     where param_len's bit7=1(0x80), it indicates delay 120~150ms

/* Sequence for ST7789 (Note: differences between manufacturers) */
#if ((DRV_LCD1==IC_ST7789) || (DRV_LCD2==IC_ST7789))
static const uint8_t init_seq_ST7789[] = {
    //0x01, 0x80,       // SWRESET: 150ms delay, replace with RESET-PIN
    //0x36, 0x01, 0x00, // MADCTL: Portrait mode, replace with lcd_set_madctl(mode)
    0x3A, 0x01, 0x05,   // COLMOD: 16-bit/pixel (RGB565)
    0xB2, 0x05, 0x0C, 0x0C, 0x00, 0x33, 0x33, // PORCTRK: Frame rate control
    0xB7, 0x01, 0x35,   // GCTRL: VGHV/VGLV voltage control
    0xBB, 0x01, 0x19,   // VCOMS: VCOM offset voltage
    0xC0, 0x01, 0x2C,   // LCMCTRL: LCD control interface
    0xC2, 0x01, 0x01,   //0xFF, // VDVVRHEN: VDV settings
    0xC3, 0x01, 0x12,   // VRHS: VCOM voltage step
    0xC4, 0x01, 0x20,   // VDVS: VCOM offset
    0xC6, 0x01, 0x0F,   // FRCTRL2: 60Hz frame rate
    0xD0, 0x02, 0xA4, 0xA1, // PWCTRL1: Power control
    
    0xE0, 0x0E, 0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23, // PVGAMCTRL: Positive gamma
    0xE1, 0x0E, 0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23, // NVGAMCTRL: Negative gamma

    0x21, 0x00,         // INVON: Display inversion
    //0x11, 0x80,       // SLPOUT: 120ms delay
    //0x29, 0x80,       // DISPON: 120ms delay, replace with lcd_disp_on()
    0x00,
};
#else
#define init_seq_ST7789     NULL
#endif

/* Sequence for GC9A01 (Note: differences between manufacturers) */
#if ((DRV_LCD1==IC_GC9A01) || (DRV_LCD2==IC_GC9A01))
static const uint8_t init_seq_GC9A01[] = {
    0xFE, 0x00,
    0xEF, 0x00,
    0xEB, 0x01, 0x14,
    /* Set power */
    0x84, 0x01, 0x40,
    0x85, 0x01, 0xF1,
    0x86, 0x01, 0x98,
    0x87, 0x01, 0x28,
    0x88, 0x01, 0x0A,
    0x89, 0x01, 0x21,
    0x8A, 0x01, 0x00,
    0x8B, 0x01, 0x80,
    0x8C, 0x01, 0x01,
    0x8D, 0x01, 0x01,
    0x8E, 0x01, 0xDF,
    0x8F, 0x01, 0x52,
    /* Set display*/
    0xB6, 0x02, 0x00, 0x20,
    //0x36, 0x01, 0x08, // MADCTL: Portrait mode with RGB Inv, replace with lcd_set_madctl(mode)
    0x3A, 0x01, 0x05,
    0x90, 0x04, 0x08, 0x08, 0x08, 0x08,
    0xE8, 0x01, 0x34,
    0xFF, 0x03, 0x60, 0x01, 0x04,

    0xC3, 0x01, 0x14,
    0xC4, 0x01, 0x14,
    0xC9, 0x01, 0x25,
    0xBE, 0x01, 0x11,
    0xBC, 0x01, 0x00,
    0xE1, 0x02, 0x10, 0x0E,

    0xDF, 0x03, 0x21, 0x0C, 0x02,
    0xED, 0x02, 0x1B, 0x0B,
    0xAE, 0x01, 0x77,
    0xCD, 0x01, 0x63,
    /* Set gamma */
    0x70, 0x09, 0x07, 0x07, 0x04, 0x0E, 0x0F, 0x09, 0x07, 0x08, 0x03,
    0xF0, 0x06, 0x45, 0x09, 0x08, 0x08, 0x26, 0x2A,
    0xF1, 0x06, 0x43, 0x70, 0x72, 0x36, 0x37, 0x6F,
    0xF2, 0x06, 0x45, 0x09, 0x08, 0x08, 0x26, 0x2A,
    0xF3, 0x06, 0x43, 0x70, 0x72, 0x36, 0x37, 0x6F,

    0x62, 0x0C, 0x18, 0x0D, 0x71, 0xED, 0x70, 0x70, 0x18, 0x0F, 0x71, 0xEF, 0x70, 0x70,
    0x63, 0x0C, 0x18, 0x11, 0x71, 0xF1, 0x70, 0x70, 0x18, 0x13, 0x71, 0xF3, 0x70, 0x70,
    0x64, 0x07, 0x28, 0x29, 0xF1, 0x01, 0xF1, 0x00, 0x07,
    0x66, 0x0A, 0x3C, 0x00, 0xCD, 0x67, 0x45, 0x45, 0x10, 0x00, 0x00, 0x00,
    0x67, 0x0A, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x01, 0x54, 0x10, 0x32, 0x98,

    0x74, 0x07, 0x10, 0x85, 0x80, 0x00, 0x00, 0x4E, 0x00,
    0x98, 0x02, 0x3E, 0x07,
    0xBA, 0x01, 0x80,

    0x35, 0x00,
    0x21, 0x00,
    //0x11, 0x80,
    //0x29, 0x80,       // DISPON: 120ms delay, replace with lcd_disp_on()
    0x00,
};
#else
#define init_seq_GC9A01     NULL
#endif

#define INIT_SEQ_DRV(drv)   ((drv==IC_ST7789) ? init_seq_ST7789 : init_seq_GC9A01)


/*
 * LOCAL FUNCTIONS
 ****************************************************************************************
 */

static inline void lcd_hal_init(void)
{
    // pads set as output
    GPIO_DIR_SET_HI(  (1UL<<PA_LCD_RST) | (1UL<<PA_LCD_BLK)
                    | (1UL<<PA_LCD_CS1) | (1UL<<PA_LCD_DC1)
                    | (1UL<<PA_LCD_CS2) | (1UL<<PA_LCD_DC2) );

    // pads set as spim(CLK MOSI)
    csc_output(PA_LCD_CLK, CSC_SPIM_CLK);
    iom_ctrl(PA_LCD_CLK, IOM_SEL_CSC | IOM_DRV_LVL1 | IOM_PULLUP);

    csc_output(PA_LCD_SDA, CSC_SPIM_MOSI);
    iom_ctrl(PA_LCD_SDA, IOM_SEL_CSC | IOM_DRV_LVL1 | IOM_PULLUP);

    // spim reset and configure as msb-single-TX
    RCC_AHBCLK_EN(AHB_SPIM_BIT);
    RCC_AHBRST_REQ(AHB_SPIM_BIT);
    spim_conf(SPIM_CR_MSB_FST_BIT | SPIM_CR_TX_EN_BIT | SPIM_CR_TX_DMA_BIT);

    // enable SPI_TX DMA Channel Interrupt
    dma_init();
    DMACHNL_INT_EN(DMA_CH_SPI_TX);

    NVIC_EnableIRQ(DMAC_IRQn);
    DMA_SPIM_TX_INIT(DMA_CH_SPI_TX);
}

static inline void lcd_hal_rst(void)
{
    // Reset(>=10ms), wait Ready(>=120ms)
    GPIO_DAT_CLR(1UL << PA_LCD_RST);
    delay_ms(20);

    GPIO_DAT_SET(1UL << PA_LCD_RST);
    delay_ms(120);
}

static inline void lcd_dc_high(void)
{
    #if (PA_LCD_DC1 == PA_LCD_DC2)
    GPIO_DAT_SET(1UL << PA_LCD_DC1);
    #else
    uint8_t id = lcd_env.selId;

    if (id & ID_LCD1) GPIO_DAT_SET(1UL << PA_LCD_DC1);
    if (id & ID_LCD2) GPIO_DAT_SET(1UL << PA_LCD_DC2);
    #endif
}

static inline void lcd_dc_low(void)
{
    #if (PA_LCD_DC1 == PA_LCD_DC2)
    GPIO_DAT_CLR(1UL << PA_LCD_DC1);
    #else
    uint8_t id = lcd_env.selId;

    if (id & ID_LCD1) GPIO_DAT_CLR(1UL << PA_LCD_DC1);
    if (id & ID_LCD2) GPIO_DAT_CLR(1UL << PA_LCD_DC2);
    #endif
}

static inline void lcd_cs_high(void)
{
    #if (PA_LCD_CS1 == PA_LCD_CS2)
    GPIO_DAT_SET(1UL << PA_LCD_CS1);
    #else
    uint8_t id = lcd_env.selId;

    if (id & ID_LCD1) GPIO_DAT_SET(1UL << PA_LCD_CS1);
    if (id & ID_LCD2) GPIO_DAT_SET(1UL << PA_LCD_CS2);
    #endif
}

static inline void lcd_cs_low(void)
{
    #if (PA_LCD_CS1 == PA_LCD_CS2)
    GPIO_DAT_CLR(1UL << PA_LCD_CS1);
    #else
    uint8_t id = lcd_env.selId;

    if (id & ID_LCD1) GPIO_DAT_CLR(1UL << PA_LCD_CS1);
    if (id & ID_LCD2) GPIO_DAT_CLR(1UL << PA_LCD_CS2);
    #endif
}

static void lcd_write_cmd(uint8_t cmd, uint8_t plen, const uint8_t *param)
{
    // DC=0(cmd), CS=0
    lcd_dc_low();
    lcd_cs_low();

    // Send cmd
    SPIM->STATUS_CLR.Word = SPIM_STATUS_CLR_ALL;

    while (SPIM->STATUS.SPIM_TX_FFULL);
    SPIM->TX_DATA = cmd;

    while (SPIM->STATUS.SPIM_BUSY);

    // DC=1(data)
    lcd_dc_high();

    // Send params 
    for (uint8_t txn = 0; txn < plen; txn++) {
        while (SPIM->STATUS.SPIM_TX_FFULL);
        SPIM->TX_DATA = param[txn];
    }

    while (SPIM->STATUS.SPIM_BUSY);
    SPIM->STATUS_CLR.Word = SPIM_STATUS_CLR_ALL;

    // CS=1
    lcd_cs_high();
}

static /**
 ****************************************************************************************
 * @brief Initialize LCD controller with configured orientation
 * @param[in]  void
 ****************************************************************************************
 */
void lcd_init_seq(const uint8_t *seq)
{
    if (seq == NULL) return;

    while (*seq != 0x00)
    {
        uint8_t cmd = *seq++;
        uint8_t tmp = *seq++;
        uint8_t len = tmp & 0x7F;

        // write cmd with delay if need
        lcd_write_cmd(cmd, len, seq);
        if (tmp & 0x80) {
            delay_ms(150);
        }

        // goto next cmd
        seq += len;
    }
}

static void lcd_disp_on(void)
{
    lcd_write_cmd(CMD_SLPOUT, 0, NULL);
    delay_ms(MS_SLPOUT);

    lcd_write_cmd(CMD_DISPON, 0, NULL);
    delay_ms(MS_DISPON);
}

static void lcd_set_madctl(uint8_t mode)
{
    lcd_write_cmd(CMD_MADCTL, 1, &mode);
}

static void lcd_set_vsaddr(uint16_t addr)
{
    uint8_t buf[2];

    buf[0] = (addr >> 8) & 0xFF; /* start address msb */
    buf[1] = (addr >> 0) & 0xFF; /* start address lsb */
    lcd_write_cmd(CMD_VSCRSADD, 2, buf);
}

static void lcd_set_window(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    uint8_t axis[4];

    // adjust end-XY
    uint16_t xe = x + w - 1;
    uint16_t ye = y + h - 1;

    // X-axis (Column)
    axis[0] = (x  >> 8) & 0xFF;
    axis[1] = (x  >> 0) & 0xFF;
    axis[2] = (xe >> 8) & 0xFF;
    axis[3] = (xe >> 0) & 0xFF;
    lcd_write_cmd(CMD_CASET, 4, axis);

    // Y-axis (Row)
    axis[0] = (y  >> 8) & 0xFF;
    axis[1] = (y  >> 0) & 0xFF;
    axis[2] = (ye >> 8) & 0xFF;
    axis[3] = (ye >> 0) & 0xFF;
    lcd_write_cmd(CMD_RASET, 4, axis);

    // Memory Write
    lcd_write_cmd(CMD_RAMWR, 0, NULL);
}

/// Read flash data in words, faster than XIP
__SRAMFN static void flash_dread(uint32_t offset, uint32_t *buff, uint32_t wlen)
{
    GLOBAL_INT_DISABLE();

    while (SYSCFG->ACC_CCR_BUSY) ;

    uint32_t reg_val = (CACHE->CCR.Word);
    CACHE->CCR.Word  = 0;
    CACHE->CIR.Word  = (0x01 << CACHE_INV_ALL_POS);

    fshc_read(offset, buff, wlen, FCM_MODE_DUAL | FSH_CMD_DLRD);
    //memcpy(buff, (uint32_t *)offset, wlen);

    CACHE->CCR.Word = reg_val;

    GLOBAL_INT_RESTORE();
}

/*__STATIC_FORCEINLINE*/
__SRAMFN static void dma_spi_tx_done(void)
{
    // if finished, release
    if (--lcd_env.irq_cnt == 0) {
        lcd_env.state = LS_IDLE;
        // wait spi finish, CS=1
        while (SPIM->STATUS.SPIM_BUSY); 
        SPIM->STATUS_CLR.Word = SPIM_STATUS_CLR_ALL;
        lcd_cs_high();
        return;
    }

    // if more data, continue transmit
    if (lcd_env.len_rem > 0) {
        uint8_t *p_buf;
        uint8_t  chidx;
        uint16_t dma_len;

        // find the finished channel
        if (DMA->PRIALT_SET & (1UL << DMA_CH_SPI_TX)) {
            chidx = DMA_CH_SPI_TX;
            p_buf = dma_buff[0];
        } else {
            chidx = DMA_CH_SPI_TX | DMA_CH_ALT;
            p_buf = dma_buff[1];
        }

        // read remain data from flash
        dma_len = (lcd_env.len_rem > DMA_LEN_MAX) ? DMA_LEN_MAX : lcd_env.len_rem;
        flash_dread(lcd_env.dat_pos, (uint32_t *)p_buf, ALIGN4_HI_WL(dma_len));
        lcd_env.dat_pos += dma_len;
        lcd_env.len_rem -= dma_len;

        // config channel
        DMA_SPIM_TX_CONF(chidx, p_buf, dma_len, CCM_PING_PONG);
        return;
    }
}

__SRAMFN void DMAC_IRQHandler(void)
{
    uint32_t iflag = DMACHNL_INT_GET(DMA_CH_SPI_TX);

    if (iflag)
    {
        // disable intr
        //DMACHNL_INT_DIS(DMA_CH_SPI_TX);

        // clear intr flag
        DMACHNL_INT_CLR(DMA_CH_SPI_TX);

        dma_spi_tx_done();

        // re-enable intr
        //DMACHNL_INT_EN(DMA_CH_SPI_TX);
    }
}


/*
 * EXPORT FUNCTIONS
 ****************************************************************************************
 */

uint8_t lcd_get_state(void)
{
    return lcd_env.state;
}

void lcd_wait_done(void)
{
    while (lcd_env.state == LS_BUSY);
}

bool lcd_fill_color(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t rgb565)
{
    // Set axis window
    lcd_set_window(x, y, w, h);

    // Write memory in MSB order
    lcd_cs_low();
    for (uint16_t i = 0; i < h; i++) {
        for (uint16_t j = 0; j < w; j++) {
            while (SPIM->STATUS.SPIM_TX_FFULL);
            SPIM->TX_DATA = (rgb565 >> 8) & 0xFF; //msb
            while (SPIM->STATUS.SPIM_TX_FFULL);
            SPIM->TX_DATA = (rgb565 >> 0) & 0xFF; //lsb
        }

        while (SPIM->STATUS.SPIM_BUSY);
        SPIM->STATUS_CLR.Word = SPIM_STATUS_CLR_ALL;
    }

    lcd_cs_high();

    return true;
}

bool lcd_fill_pixel(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *pixel)
{
    // Set axis window
    lcd_set_window(x, y, w, h);

    // Write memory in MSB order
    lcd_cs_low();

    for (uint16_t i = 0; i < h; i++) {
        for (uint16_t j = 0; j < w; j++) {
            while (SPIM->STATUS.SPIM_TX_FFULL);
            SPIM->TX_DATA = (*pixel >> 8) & 0xFF; //msb
            while (SPIM->STATUS.SPIM_TX_FFULL);
            SPIM->TX_DATA = (*pixel >> 0) & 0xFF; //lsb

            pixel++;
        }

        while (SPIM->STATUS.SPIM_BUSY);
        SPIM->STATUS_CLR.Word = SPIM_STATUS_CLR_ALL;
    }

    lcd_cs_high();

    return true;
}

bool lcd_draw_image(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const void *pic)
{
    // check pic exist and aligned(4) in flash
    if (w==0 || h==0 || ((uint32_t)pic & 0x3)
        || (((uint32_t)pic >> 24) != (FLASH_BASE >> 24))) {
        return false;
    }

    if (lcd_env.state == LS_BUSY) {
        return  false;
    }

    uint32_t pic_len = w * h * 2;
    uint16_t irq_cnt = (pic_len + (DMA_LEN_MAX - 1)) / DMA_LEN_MAX;
    uint16_t dma_len = (irq_cnt >= 2) ? (2*DMA_LEN_MAX) : pic_len;
    
    // read data from flash
    uint32_t offset = PDATA_OFFSET(pic);
    flash_dread(offset, (uint32_t *)dma_buff[0], ALIGN4_HI_WL(dma_len));
    // record remain data info
    lcd_env.dat_pos = offset + dma_len;
    lcd_env.len_rem = pic_len - dma_len;
    lcd_env.irq_cnt = irq_cnt;
    lcd_env.state = LS_BUSY;

    // set aixs window
    lcd_set_window(x, y, w, h);
    lcd_cs_low();

    // config DMA to transmit
    SPIM->CTRL.SPIM_TX_DMA_EN = 0;

    if (irq_cnt > 1/*dma_len > DMA_LEN_MAX*/) {
        // DMA Ping-Pong mode
        DMA_SPIM_TX_CONF(DMA_CH_SPI_TX, dma_buff[0], DMA_LEN_MAX, CCM_PING_PONG);
        DMA_SPIM_TX_CONF(DMA_CH_SPI_TX | DMA_CH_ALT, dma_buff[1], (dma_len - DMA_LEN_MAX), CCM_PING_PONG);
    } else {
        // DMA Basic mode
        DMA_SPIM_TX_CONF(DMA_CH_SPI_TX, dma_buff[0], dma_len, CCM_BASIC);
    }

    SPIM->CTRL.SPIM_TX_DMA_EN = 1;
    return true;
}

bool lcd_select(int id)
{
    if (lcd_env.state == LS_BUSY) {
        return false;
    }
    lcd_env.selId = id;
    return true;
}

void lcd_reset(void)
{
    LCD_BLK_OFF();

    if (lcd_env.state == LS_BUSY) {
        // disable TX_DMA
        SPIM->CTRL.SPIM_TX_DMA_EN = 0;
        DMA->CHNL_EN_CLR = (1UL << DMA_CH_SPI_TX);
    }
    lcd_env.state = LS_IDLE;

    lcd_hal_rst();

    // Set init-code
    #if (DRV_LCD1 == DRV_LCD2)
    lcd_select(ID_LCDS);
    lcd_init_seq(INIT_SEQ_DRV(DRV_LCD1));
    #else
    lcd_select(ID_LCD1);
    lcd_init_seq(INIT_SEQ_DRV(DRV_LCD1));
    #if (DRV_LCD2)
    lcd_select(ID_LCD2);
    lcd_init_seq(INIT_SEQ_DRV(DRV_LCD2));
    #endif //(DRV_LCD2)
    #endif //(DRV_LCD1 == DRV_LCD2)

    // Set direction
    lcd_select(ID_LCD1);
    lcd_set_madctl(MAD_LCD1);
    #if (VSA_LCD1)
    lcd_set_vsaddr(VSA_LCD1);
    #endif

    #if (DRV_LCD2)
    lcd_select(ID_LCD2);
    lcd_set_madctl(MAD_LCD2);
    #if (VSA_LCD2)
    lcd_set_vsaddr(VSA_LCD2);
    #endif
    #endif //(DRV_LCD2)

    // Display on
    lcd_select(ID_LCDS);
    lcd_disp_on();

    // Set background
    lcd_fill_color(0, 0, LCD_WIDTH, LCD_HEIGHT, BG_COLOR);

    LCD_BLK_ON();
}

void lcd_init(void)
{
    memset(&lcd_env, 0, sizeof(lcd_env_t));

    lcd_hal_init();

    lcd_reset();
}
