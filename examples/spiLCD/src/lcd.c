#include "lcd.h"
#include "b6x.h"
#include "drvs.h"
#include "regs.h"

#define LCD_RS_PAD        (13)
#define LCD_RESET_PAD     (12)
//#define LCD_RS_PAD        (10)
//#define LCD_RESET_PAD     (16)
#define LCD_CS_PAD        (14)
#define LCD_CLK_PAD       (15)
#define LCD_SDA_PAD       (17)
#define LCD_BACKLIGHT_PAD (18)

#define DMA_CHNL_TX 0
#define DMA_CHNL_RX 1

#define SPIM_DC_H(pad) GPIO_DAT_SET(1UL << (pad))
#define SPIM_DC_L(pad) GPIO_DAT_CLR(1UL << (pad))

#define SPIM_RST_H(pad) GPIO_DAT_SET(1UL << (pad))
#define SPIM_RST_L(pad) GPIO_DAT_CLR(1UL << (pad))

#define SPIM_IO_INIT(pad) dowl(GPIO_DAT_SET(1UL << (pad)); GPIO_DIR_SET(1UL << (pad));)

#define LCD_SEND(cmd, ...)                                                                         \
    do                                                                                             \
    {                                                                                              \
        uint8_t params[] = {                                                                       \
            __VA_ARGS__,                                                                           \
        };                                                                                         \
        lcd_spi_cmd((cmd), params, sizeof(params));                                                \
    } while (0)

extern const unsigned char gImage_steam[];
extern const unsigned char gImage_steam_font[];
extern const unsigned char gImage_coffee[];
extern const unsigned char gImage_coffee_font[];
extern const unsigned char gImage_hot_water[];
extern const unsigned char gImage_hot_water_font[];

static inline void lcd_spi_init(void)
{
    SPIM_CS_INIT(LCD_CS_PAD);
    SPIM_IO_INIT(LCD_RS_PAD);
    SPIM_IO_INIT(LCD_RESET_PAD);
    SPIM_IO_INIT(LCD_BACKLIGHT_PAD);

    // i2c_clk_en rst_req or soft_reset
    RCC_AHBCLK_EN(AHB_SPIM_BIT);
    RCC_AHBRST_REQ(AHB_SPIM_BIT);

    // CLK PAD
    csc_output(LCD_CLK_PAD, CSC_SPIM_CLK);
    iom_ctrl(LCD_CLK_PAD, IOM_SEL_CSC | IOM_DRV_LVL1 | IOM_PULLUP);

    // MOSI PAD
    csc_output(LCD_SDA_PAD, CSC_SPIM_MOSI);
    iom_ctrl(LCD_SDA_PAD, IOM_SEL_CSC | IOM_DRV_LVL1 | IOM_PULLUP);

#if (SPI_MCU_MODE)
    spim_conf(SPIM_CR_MSB_FST_BIT | SPIM_CR_TX_EN_BIT /* | SPIM_CR_TX_DMA_BIT*/);
#else
    dma_init();
    spim_conf(SPIM_CR_MSB_FST_BIT | SPIM_CR_TX_EN_BIT | SPIM_CR_TX_DMA_BIT);

    // enable SPI TX DMA Channel Interrupt
    DMACHNL_INT_EN(DMA_CHNL_TX);

    NVIC_EnableIRQ(DMAC_IRQn);
    DMA_SPIM_TX_INIT(DMA_CHNL_TX);
#endif
}

static void lcd_spi_cmd(uint8_t cmd, uint8_t *param, int length)
{
    int txn;

    SPIM_CS_L(LCD_CS_PAD);
    SPIM_DC_L(LCD_RS_PAD);

    SPIM->STATUS_CLR.Word = SPIM_STATUS_CLR_ALL;

    while (SPIM->STATUS.SPIM_TX_FFULL)
        ;

    SPIM->TX_DATA = cmd;

    while (SPIM->STATUS.SPIM_BUSY)
        ;

    SPIM_DC_H(LCD_RS_PAD);

    for (txn = 0; txn < length;)
    {
        if (SPIM->STATUS.SPIM_TX_FFULL == 0)
        {
            SPIM->TX_DATA = param[txn++];
        }
    }

    while (SPIM->STATUS.SPIM_BUSY)
        ;

    SPIM->STATUS_CLR.Word = SPIM_STATUS_CLR_ALL;

    SPIM_CS_H(LCD_CS_PAD);
}

static void lcd_spi_fill(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t pixel)
{
    int     remaining = (x1 - x0) * (y1 - y0) * 2;
    int     txn_max;
    uint8_t pixel_low  = pixel & 0xFF;
    uint8_t pixel_high = (pixel >> 8) & 0xFF;

    LCD_SEND(0x2A, x0 >> 8, x0 & 0xff, (x1 - 1) >> 8, (x1 - 1) & 0xff);
    LCD_SEND(0x2B, y0 >> 8, y0 & 0xff, (y1 - 1) >> 8, (y1 - 1) & 0xff);

    lcd_spi_cmd(0x2C, NULL, 0);

    SPIM_CS_L(LCD_CS_PAD);

    while (remaining > 0)
    {
        // 确保每次传输长度不超过 65534（最大偶数字节）
        txn_max  = remaining > UINT16_MAX ? UINT16_MAX : remaining;
        txn_max &= ~0x1; // 强制转换为偶数

        for (int txn = 0; txn < txn_max; txn += 2)
        {
            while (SPIM->STATUS.SPIM_TX_FFULL)
                ;

            SPIM->TX_DATA = pixel_high;

            while (SPIM->STATUS.SPIM_TX_FFULL)
                ;

            SPIM->TX_DATA = pixel_low;
        }

        while (SPIM->STATUS.SPIM_BUSY)
            ;

        SPIM->STATUS_CLR.Word = SPIM_STATUS_CLR_ALL;

        remaining -= txn_max;
    }

    SPIM_CS_H(LCD_CS_PAD);
}

static void lcd_spi_bitmap(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t *pixel)
{
    int remaining = (x1 - x0) * (y1 - y0) * 2;
    int txn_max;

    LCD_SEND(0x2A, x0 >> 8, x0 & 0xff, (x1 - 1) >> 8, (x1 - 1) & 0xff);
    LCD_SEND(0x2B, y0 >> 8, y0 & 0xff, (y1 - 1) >> 8, (y1 - 1) & 0xff);

    lcd_spi_cmd(0x2C, NULL, 0);

    SPIM_CS_L(LCD_CS_PAD);

    while (remaining > 0)
    {
        // 确保每次传输长度不超过 65534（最大偶数字节）
        txn_max  = remaining > UINT16_MAX ? UINT16_MAX : remaining;
        txn_max &= ~0x1; // 强制转换为偶数

        for (int txn = 0; txn < txn_max; txn += 2)
        {
            while (SPIM->STATUS.SPIM_TX_FFULL)
                ;

            SPIM->TX_DATA = pixel[txn + 1];

            while (SPIM->STATUS.SPIM_TX_FFULL)
                ;

            SPIM->TX_DATA = pixel[txn];
        }

        while (SPIM->STATUS.SPIM_BUSY)
            ;

        SPIM->STATUS_CLR.Word = SPIM_STATUS_CLR_ALL;

        remaining -= txn_max;
    }

    SPIM_CS_H(LCD_CS_PAD);
}

#if (SPI_MCU_MODE)
static void lcd_set_pic(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t *pic)
{
    int remaining = (x1 - x0) * (y1 - y0) * 2;
    int txn_max;
    int count = 0;

    LCD_SEND(0x2A, x0 >> 8, x0 & 0xff, (x1 - 1) >> 8, (x1 - 1) & 0xff);
    LCD_SEND(0x2B, y0 >> 8, y0 & 0xff, (y1 - 1) >> 8, (y1 - 1) & 0xff);

    lcd_spi_cmd(0x2C, NULL, 0);

    SPIM_CS_L(LCD_CS_PAD);

    while (remaining > 0)
    {
        // 确保每次传输长度不超过 65534（最大偶数字节）
        txn_max = remaining > UINT16_MAX ? UINT16_MAX - 1 : remaining;

        for (int txn = 0; txn < txn_max; txn += 2)
        {
            while (SPIM->STATUS.SPIM_TX_FFULL)
                ;

            SPIM->TX_DATA = pic[txn + count];

            while (SPIM->STATUS.SPIM_TX_FFULL)
                ;

            SPIM->TX_DATA = pic[txn + 1 + count];
        }

        while (SPIM->STATUS.SPIM_BUSY)
            ;

        SPIM->STATUS_CLR.Word = SPIM_STATUS_CLR_ALL;

        remaining -= txn_max;
        count     += txn_max;
    }

    SPIM_CS_H(LCD_CS_PAD);
}
#else

#define DMA_CHNL_CTRL(chidx)       (DMA_CHNL_CTRL_Typedef *)(DMA->CTRLBASE_POINTER.Word + ((chidx) * sizeof(DMA_CHNL_CTRL_Typedef)))

bool dma_chnl_reload_len(uint8_t chidx, uint16_t len)
{
    DMA_CHNL_CTRL_Typedef *chnl_cur;

    chidx = chidx % DMA_CH_MAX; // Pri Channel

    bool alter = (DMA->PRIALT_SET & (1UL << chidx)) ? true : false;

    chnl_cur = DMA_CHNL_CTRL(chidx);

    if (!alter)
    {
        chnl_cur = DMA_CHNL_CTRL(chidx | DMA_CH_ALT);
    }
    
    chnl_cur->SRC_DATA_END_PTR = chnl_cur->SRC_DATA_END_PTR - chnl_cur->TRANS_CFG_RESV.N_MINUS_1;
    
    chnl_cur->TRANS_CFG_RESV.N_MINUS_1 = len - 1;
    chnl_cur->SRC_DATA_END_PTR += chnl_cur->TRANS_CFG_RESV.N_MINUS_1;
    
    chnl_cur->TRANS_CFG_DATA = chnl_cur->TRANS_CFG_RESV;

    DMA->CHNL_EN_SET = (1UL << chidx);
    
    return alter;
}

#define delay_ms(n)          bootDelayMs(n)

// Channel primary
#define DMA_CHNL_SPI_TX_P    (DMA_CHNL_TX)
// Channel alternate
#define DMA_CHNL_SPI_TX_A    (DMA_CHNL_SPI_TX_P | DMA_CH_ALT)
#define DMA_TRANS_MAX_LEN    (1024)

/// Align value on the multiple of 4 equal or nearest higher.
/// @param[in] val Value to align.
#define ALIGN4_HI(_val)      (((_val) + 3) & ~3)
// byte len to word len.
#define ALIGN4_HI_WL(_val)   ((((_val) + 3) & ~3) >> 2)

// flash data pointer to flash offset
#define PDATA_OFFSET(p_data) ((uint32_t)(p_data)-FLASH_BASE)

// #define PDATA_OFFSET(p_data) (uint32_t)(p_data)

struct dma_data_tag
{
    uint32_t dma_ping_len : 11;
    uint32_t dma_pong_len : 11;
    uint32_t dma_irq_cnt  : 10;
};

struct bsp_spi_env_tag
{
    volatile uint32_t src_data;
    volatile uint32_t remaining_len;
    volatile uint32_t dma_ping_len;
    volatile uint32_t dma_pong_len;
    volatile uint32_t dma_irq_cnt ;

//    volatile struct dma_data_tag dma_data;

    uint8_t dma_ping_buff[DMA_TRANS_MAX_LEN];
    uint8_t dma_pong_buff[DMA_TRANS_MAX_LEN];
} __DATA_ALIGNED(4);

struct bsp_spi_env_tag bsp_spi_env;

#define PING_BUFF  bsp_spi_env.dma_ping_buff
#define PONG_BUFF  bsp_spi_env.dma_pong_buff
#define REMAIN_LEN bsp_spi_env.remaining_len
#define DATA_POS   bsp_spi_env.src_data
//#define PING_LEN   bsp_spi_env.dma_data.dma_ping_len
//#define PONG_LEN   bsp_spi_env.dma_data.dma_pong_len
//#define IRQ_CNT    bsp_spi_env.dma_data.dma_irq_cnt
#define PING_LEN   bsp_spi_env.dma_ping_len
#define PONG_LEN   bsp_spi_env.dma_pong_len
#define IRQ_CNT    bsp_spi_env.dma_irq_cnt
// 0x3B
__SRAMFN void flash_dread(uint32_t offset, uint32_t *buff, uint32_t wlen)
{
    GLOBAL_INT_DISABLE();

    while (SYSCFG->ACC_CCR_BUSY)
        ;

    uint32_t reg_val = (CACHE->CCR.Word);
    CACHE->CCR.Word  = 0;
    CACHE->CIR.Word  = (0x01 << CACHE_INV_ALL_POS);

    fshc_read(offset, buff, wlen, FCM_MODE_DUAL | FSH_CMD_DLRD);
    //		memcpy(buff, (uint32_t *)offset, wlen);

    CACHE->CCR.Word = reg_val;

    GLOBAL_INT_RESTORE();
}

bool bsp_spi_done(void)
{
    if ((IRQ_CNT == 0) && (REMAIN_LEN == 0))
    {
        spim_wait();
        SPIM_CS_H(LCD_CS_PAD);

        return true;
    }

    return false;
}

static void lcd_dma_set_bitmap(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t *bitmap)
{
    int remaining = (x1 - x0) * (y1 - y0) * 2;
    int txn_max, count = 0;

    dma_chnl_ctrl(DMA_CHNL_SPI_TX_P, CHNL_DIS);

    LCD_SEND(0x2A, x0 >> 8, x0 & 0xff, (x1 - 1) >> 8, (x1 - 1) & 0xff);
    LCD_SEND(0x2B, y0 >> 8, y0 & 0xff, (y1 - 1) >> 8, (y1 - 1) & 0xff);
    lcd_spi_cmd(0x2C, NULL, 0);

    SPIM_DC_H(LCD_RS_PAD);
    SPIM_CS_L(LCD_CS_PAD);

    while (remaining > 0)
    {
        txn_max = remaining > 1024 ? 1024 : remaining;

        DMA_SPIM_TX_CONF(DMA_CHNL_TX, &bitmap[count], txn_max, CCM_BASIC);
        dma_chnl_ctrl(DMA_CHNL_TX, CHNL_EN);
        spim_wait();
        dma_chnl_ctrl(DMA_CHNL_TX, CHNL_DIS);
        remaining -= txn_max;
        count     += txn_max;
    }
    SPIM_CS_H(LCD_CS_PAD);
}

static void lcd_set_pic(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t *pic)
{
    if (!bsp_spi_done())
    {
        return;
    }

    uint32_t len = (x1 - x0) * (y1 - y0) * 2;

    SPIM->CTRL.SPIM_TX_DMA_EN = 0;

    dma_chnl_ctrl(DMA_CHNL_SPI_TX_P, CHNL_DIS);

    LCD_SEND(0x2A, x0 >> 8, x0 & 0xff, (x1 - 1) >> 8, (x1 - 1) & 0xff);
    LCD_SEND(0x2B, y0 >> 8, y0 & 0xff, (y1 - 1) >> 8, (y1 - 1) & 0xff);
    lcd_spi_cmd(0x2C, NULL, 0);
    SPIM_CS_L(LCD_CS_PAD);

    DMA->PRIALT_CLR = (1UL << DMA_CHNL_SPI_TX_P);
    IRQ_CNT         = (len + (DMA_TRANS_MAX_LEN - 1)) / DMA_TRANS_MAX_LEN;

    if (len >= (2 * DMA_TRANS_MAX_LEN))
    {
        PING_LEN = DMA_TRANS_MAX_LEN;
        PONG_LEN = DMA_TRANS_MAX_LEN;
        DATA_POS = PDATA_OFFSET(pic);

        flash_dread(DATA_POS, (uint32_t *)PING_BUFF, ALIGN4_HI_WL(PING_LEN));
        DATA_POS += PING_LEN;
        flash_dread(DATA_POS, (uint32_t *)PONG_BUFF, ALIGN4_HI_WL(PONG_LEN));
        DATA_POS += PONG_LEN;

        REMAIN_LEN = len - (PING_LEN + PONG_LEN);

        DMA_SPIM_TX_CONF(DMA_CHNL_SPI_TX_P, PING_BUFF, PING_LEN, CCM_PING_PONG);
        DMA_SPIM_TX_CONF(DMA_CHNL_SPI_TX_A, PONG_BUFF, PONG_LEN, CCM_PING_PONG);
    }
    else if (len > DMA_TRANS_MAX_LEN)
    {
        PING_LEN = DMA_TRANS_MAX_LEN;
        PONG_LEN = len - DMA_TRANS_MAX_LEN;

        DATA_POS = PDATA_OFFSET(pic);

        flash_dread(DATA_POS, (uint32_t *)PING_BUFF, ALIGN4_HI_WL(PING_LEN));
        DATA_POS += PING_LEN;
        flash_dread(DATA_POS, (uint32_t *)PONG_BUFF, ALIGN4_HI_WL(PONG_LEN));
        DATA_POS += PONG_LEN;

        REMAIN_LEN = len - (PING_LEN + PONG_LEN);

        DMA_SPIM_TX_CONF(DMA_CHNL_SPI_TX_P, PING_BUFF, PING_LEN, CCM_PING_PONG);
        DMA_SPIM_TX_CONF(DMA_CHNL_SPI_TX_A, PONG_BUFF, PONG_LEN, CCM_PING_PONG);
    }
    else
    {
        PING_LEN = len;

        DATA_POS = PDATA_OFFSET(pic);

        flash_dread(DATA_POS, (uint32_t *)PING_BUFF, ALIGN4_HI_WL(PING_LEN));
        DATA_POS += PING_LEN;

        REMAIN_LEN = 0;

        DMA_SPIM_TX_CONF(DMA_CHNL_SPI_TX_P, PING_BUFF, PING_LEN, CCM_BASIC);
    }

    SPIM->CTRL.SPIM_TX_DMA_EN = 1;
}

__SRAMFN __STATIC_INLINE void dma_spi_tx_done(void)
{
    --IRQ_CNT;

    if (REMAIN_LEN == 0)
    {
//        GPIO_DAT_SET(GPIO02);
//        GPIO_DAT_CLR(GPIO02);
        return;
    }
    uint16_t reload_len = REMAIN_LEN > DMA_TRANS_MAX_LEN ? DMA_TRANS_MAX_LEN : REMAIN_LEN;

    uint32_t *p_buff = (uint32_t *)PONG_BUFF;

    if (dma_chnl_reload_len(DMA_CHNL_SPI_TX_P, reload_len))
    {
        p_buff = (uint32_t *)PING_BUFF;
    }

GPIO_DAT_SET(GPIO03);
    flash_dread(DATA_POS, p_buff, ALIGN4_HI_WL(reload_len));
GPIO_DAT_CLR(GPIO03);

    DATA_POS   += reload_len;
    REMAIN_LEN -= reload_len;
}

__SRAMFN void DMAC_IRQHandler(void)
{
    uint32_t iflag = DMACHNL_INT_GET(DMA_CHNL_SPI_TX_P);
GPIO_DAT_SET(GPIO04);
    if (iflag)
    {
        // disable intr
        DMACHNL_INT_DIS(DMA_CHNL_SPI_TX_P);

        // clear intr flag
        DMACHNL_INT_CLR(DMA_CHNL_SPI_TX_P);

        dma_spi_tx_done();

        // re-enable intr
        DMACHNL_INT_EN(DMA_CHNL_SPI_TX_P);
    }
GPIO_DAT_CLR(GPIO04);
}

#endif

static void lcd_dev_startup_task()
{
    SPIM_RST_H(LCD_RESET_PAD);
    delay_ms(100);
    SPIM_RST_L(LCD_RESET_PAD);
    delay_ms(120);
    SPIM_RST_H(LCD_RESET_PAD);
    delay_ms(120);

    lcd_spi_cmd(0xFE, NULL, 0);
    lcd_spi_cmd(0xEF, NULL, 0);
    LCD_SEND(0xEB, 0x14);
    LCD_SEND(0x84, 0x40);
    LCD_SEND(0x85, 0xF1);
    LCD_SEND(0x86, 0x98);
    LCD_SEND(0x87, 0x28);

    LCD_SEND(0x88, 0x0A);
    LCD_SEND(0x89, 0x21);
    LCD_SEND(0x8A, 0x00);
    LCD_SEND(0x8B, 0x80);
    LCD_SEND(0x8C, 0x01);
    LCD_SEND(0x8D, 0x01);
    LCD_SEND(0x8E, 0xDF);
    LCD_SEND(0x8F, 0x52);
    LCD_SEND(0xB6, 0x20);

    LCD_SEND(0x36, 0x48);
    LCD_SEND(0x3A, 0x05);
    LCD_SEND(0x90, 0x08, 0x08, 0X08, 0X08);

    LCD_SEND(0xE8, 0x34);
    LCD_SEND(0xFF, 0x60, 0x01, 0x04);

    LCD_SEND(0x74, 0x10, 0x75, 0x80, 0x00, 0x00, 0x4E, 0x00);

    LCD_SEND(0xC3, 0x14);
    LCD_SEND(0xC4, 0x14);
    LCD_SEND(0xC9, 0x25);
    LCD_SEND(0xBE, 0x11);
    LCD_SEND(0xBC, 0x00);

    LCD_SEND(0xE1, 0x10, 0x0E);
    LCD_SEND(0xDF, 0x21, 0x0C, 0x02);
    LCD_SEND(0xED, 0x1B, 0x0B);
    LCD_SEND(0xAE, 0x77);
    LCD_SEND(0xCD, 0x63);

    LCD_SEND(0x70, 0x07, 0x09, 0x04, 0x0E, 0x0F, 0x09, 0x07, 0X08, 0x03);
    LCD_SEND(0xF0, 0x46, 0x09, 0x0A, 0x08, 0x05, 0x2C);
    LCD_SEND(0xF1, 0x46, 0x76, 0x76, 0x32, 0x36, 0x9F);
    LCD_SEND(0xF2, 0x46, 0x09, 0x0A, 0x08, 0x05, 0x2C);
    LCD_SEND(0xF3, 0x46, 0x76, 0x76, 0x32, 0x36, 0x9F);

    LCD_SEND(0x62, 0x18, 0x0D, 0x71, 0xED, 0x70, 0x70, 0x18, 0X0F, 0x71, 0xEF, 0x70, 0x70);
    LCD_SEND(0x63, 0x18, 0x11, 0x71, 0xF1, 0x70, 0x70, 0x18, 0X13, 0x71, 0xF3, 0x70, 0x70);
    LCD_SEND(0x64, 0x28, 0x29, 0xF1, 0x01, 0xF1, 0x00, 0x07);
    LCD_SEND(0x66, 0x3C, 0x00, 0xCD, 0x67, 0x45, 0x45, 0x10, 0X00, 0x00, 0x00);
    LCD_SEND(0x67, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x01, 0x54, 0X10, 0x32, 0x98);

    LCD_SEND(0x98, 0x3E, 0x07);
    LCD_SEND(0xBA, 0x80);
    LCD_SEND(0x35, 0x21);
    lcd_spi_cmd(0x21, NULL, 0);
    delay_ms(120);
    lcd_spi_cmd(0x11, NULL, 0);
    delay_ms(320);
    lcd_spi_cmd(0x29, NULL, 0);
    delay_ms(120);
    lcd_spi_cmd(0x2C, NULL, 0);

    delay_ms(200);

    lcd_spi_fill(0, 0, 240, 240, 0x0000);

    delay_ms(200);

#define NEXT_PIC_DLY 1000

    while (1)
    {
        lcd_set_pic(0, 76, 240, 240, (uint8_t *)gImage_steam);
#if !(SPI_MCU_MODE)
        while (!bsp_spi_done())
            ;
#endif

        lcd_set_pic(87, 30, 153, 48, (uint8_t *)gImage_steam_font);
#if !(SPI_MCU_MODE)
        while (!bsp_spi_done())
            ;
#endif
        delay_ms(NEXT_PIC_DLY);

        lcd_set_pic(0, 76, 240, 240, (uint8_t *)gImage_coffee);
#if !(SPI_MCU_MODE)
        while (!bsp_spi_done())
            ;
#endif

        lcd_set_pic(87, 30, 153, 48, (uint8_t *)gImage_coffee_font);
#if !(SPI_MCU_MODE)
        while (!bsp_spi_done())
            ;
#endif
        delay_ms(NEXT_PIC_DLY);
    }
}

void lcd_dev_init(void)
{
    lcd_spi_init();
    lcd_dev_startup_task();
}

int32_t Disp0_DrawBitmap(int16_t x, int16_t y, int16_t width, int16_t height, const uint8_t *bitmap)
{
    //		lcd_spi_bitmap(x, y, x+width, y+height, (uint8_t *)bitmap);
    return 1;
}
