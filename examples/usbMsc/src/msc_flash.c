/**
 ****************************************************************************************
 *
 * @file msc_flash.c
 *
 * @brief Function of USB MSC Flash driver
 *
 ****************************************************************************************
 */

#include "regs.h"
#include "drvs.h"
#include "usbd_msc.h"

#if (DBG_MSB_FLASH)
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

#ifndef __SRAMFN_LN
#define __SRAMFN_LN(line)     __attribute__((section("ram_func." #line)))
#endif

#ifndef __ZI_ALIGNED
#define __ZI_ALIGNED(x)       __attribute__((aligned(x), zero_init))
#endif

/* Flash Type */
#define INT_FLASH             (0)
#define SPI_FLASH             (1)

#ifndef MSC_FLASH_TYPE
#define MSC_FLASH_TYPE        INT_FLASH
#endif

/* Flash Page and Sector */
#define PAGE_SIZE             (256)     // 256Byte, aligned(4)
#define PAGE_WLEN             (64)      // 256>>2, in uint32_t
#define SECTOR_SIZE           (4096)    // 4KB

#define CO_WLEN(blen)         ((blen) >> 2) // conv len: uint8_t -> uint32_t
#define CO_BLEN(wlen)         ((wlen) << 2) // conv len: uint32_t -> uint8_t
#define PAGE_NB(blen)         (((blen) + PAGE_SIZE-1) / PAGE_SIZE)



/*
 * GLOBAL VARIABLES
 ****************************************************************************************
 */

__ZI_ALIGNED(4)
static uint32_t rd_buff[SECTOR_SIZE/sizeof(uint32_t)];


/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

#if (MSC_FLASH_TYPE == INT_FLASH) 
/* internal Flash(2Mbits, 256KB), half part */
#ifndef intFLASH_BASE
#define intFLASH_BASE         (0x20000) // high-half
#endif

#ifndef intFLASH_SIZE
#define intFLASH_SIZE         (0x20000) // 128KB
#endif

/* Wait cache idle, thene disable-flush cache */
#undef FSHC_CACHE_DISABLE
#define FSHC_CACHE_DISABLE()                        \
do {                                                \
    GLOBAL_INT_DISABLE();                           \
    while (SYSCFG->ACC_CCR_BUSY);                   \
    uint32_t reg_val = (CACHE->CCR.Word);           \
    CACHE->CCR.Word  = 0;                           \
    CACHE->CIR.Word  = (0x01 << CACHE_INV_ALL_POS);

/* Restore cache config */
#undef FSHC_CACHE_RESTORE
#define FSHC_CACHE_RESTORE()                        \
    CACHE->CCR.Word = reg_val;                      \
    GLOBAL_INT_RESTORE();                           \
} while(0)


static void msc_flash_init(void)
{
    // Nothing
}

static uint32_t msc_flash_size(void)
{
    return intFLASH_SIZE;
}

__SRAMFN_LN("msc.erase")
static void msc_flash_erase(uint32_t offset)
{
    DEBUG("offset:%X", offset);

    FSHC_CACHE_DISABLE();
    fshc_erase(intFLASH_BASE + offset, FSH_CMD_ER_SECTOR);
    FSHC_CACHE_RESTORE();
}

__SRAMFN_LN("msc.read")
static void msc_flash_read(uint32_t offset, uint32_t *buff, uint32_t wlen)
{
    DEBUG("offset:%X, len:%d", offset, wlen);

    FSHC_CACHE_DISABLE();
    fshc_read(intFLASH_BASE + offset, buff, wlen, FSH_CMD_RD); // FSH_CMD_DLRD, if need
    FSHC_CACHE_RESTORE();
}

__SRAMFN_LN("msc.write")
void msc_flash_write(uint32_t offset, uint32_t *data, uint32_t wlen)
{
    DEBUG("offset:%X, len:%d", offset, wlen);

    FSHC_CACHE_DISABLE();
    fshc_write(intFLASH_BASE + offset, data, wlen, FSH_CMD_WR);
    FSHC_CACHE_RESTORE();
}

#else //(MSB_SPI_FLASH)

/*    SOP8 SPI-Flash          */
/*        +---------+         */
/*   CS  -|1       8|- VCC    */
/*   QIO1-|2       7|- QIO3   */
/*   QIO2-|3       6|- CLK    */
/*   GND -|4       5|- QIO0   */
/*        +---------+         */
#define PIN_QCS_FLASH         2
#define PIN_QSCK_FLASH        4
#define PIN_QIO0_FLASH        5
#define PIN_QIO1_FLASH        3
#if (PIN_QIO2_QIO3)
// io mode: QSPI to set HI
#define PIN_QIO2_FLASH        16
#define PIN_QIO3_FLASH        17

#define SET_QIO2_QIO3_HI()    GPIO_DIR_SET_HI(BIT(PIN_QIO2_FLASH) | BIT(PIN_QIO3_FLASH))
#else
// hw mode: direct to VCC
#define SET_QIO2_QIO3_HI()
#endif

#define SPI_CS_L()            SPIM_CS_L(PIN_QCS_FLASH)
#define SPI_CS_H()            SPIM_CS_H(PIN_QCS_FLASH)
#define BSP_SPIM_CR           (SPIM_CR_MSB_FST_BIT | SPIM_CR_RX_EN_BIT | SPIM_CR_TX_EN_BIT)  // sys_clk/div2

#define WIP_BIT               (0x01)

typedef union flash_op_tag
{
    struct
    {
        uint32_t op_addr : 24;
        uint32_t op_cmd  : 8;
    };

    uint8_t  op_data[4];
    uint32_t word;
} flash_op_t;


static void spi_flash_transmit(uint8_t *txd, uint32_t tlen, uint8_t *rxd, uint32_t rlen)
{
    uint32_t tidx, ridx;

    SPIM->DAT_LEN  = tlen + rlen;
    SPIM->TXRX_BGN = 1;

    // send cmd
    for (tidx = 0, ridx = 0; ridx < tlen; )
    {
        if ((tidx < tlen) && (SPIM->STATUS.SPIM_TX_FFULL == 0))
        {
            SPIM->TX_DATA = txd[tidx++];
        }

        if (SPIM->STATUS.SPIM_RX_FNUM)
        {
            SPIM->RX_DATA; // drop
            ridx++;
        }
    }

    // recv rsp
    for (tidx = 0, ridx = 0; ridx < rlen; )
    {
        if ((tidx < rlen) && (SPIM->STATUS.SPIM_TX_FFULL == 0))
        {
            SPIM->TX_DATA = 0xFF;
            tidx++;
        }

        if (SPIM->STATUS.SPIM_RX_FNUM)
        {
            rxd[ridx++] = SPIM->RX_DATA;
        }
    }

    while (SPIM->STATUS.SPIM_BUSY);

    SPIM->STATUS_CLR.Word = SPIM_STATUS_CLR_ALL;
}

static void spi_flash_recv(uint8_t *fcmd, uint32_t clen, uint8_t *rsp, uint32_t rlen)
{
    GLOBAL_INT_DISABLE();
    SPI_CS_L();

    spi_flash_transmit(fcmd, clen, rsp, rlen);

    SPI_CS_H();
    GLOBAL_INT_RESTORE();
}

static void spi_flash_send(uint8_t *fcmd, uint32_t clen, uint8_t *data, uint32_t dlen)
{
    GLOBAL_INT_DISABLE();
    SPI_CS_L();

    spi_flash_transmit(fcmd, clen, NULL, 0);
    if (dlen > 0)
    {
        spi_flash_transmit(data, dlen, NULL, 0);
    }

    SPI_CS_H();
    GLOBAL_INT_RESTORE();
}

// Write Enable
static void spi_flash_wren(void)
{
    uint8_t fcmd = FSH_CMD_WR_EN;
    
    spi_flash_send(&fcmd, 1, NULL, 0);
}

// Read Status Register S07 ~ S00
static uint8_t spi_flash_sta0_get(void)
{
    uint8_t status = 0;
    uint8_t fcmd = FSH_CMD_RD_STA0;

    spi_flash_recv(&fcmd, 1, &status, 1);
    return status;
}

static void msc_flash_init(void)
{
    // spim IO-init
    SET_QIO2_QIO3_HI();
    SPIM_CS_INIT(PIN_QCS_FLASH);
    spim_init(PIN_QSCK_FLASH, PIN_QIO1_FLASH, PIN_QIO0_FLASH);

    // spim conf
    spim_conf(BSP_SPIM_CR);
}

/******************************************************
 * FlashID:
 *    ------------------------------------------------
 *   | Byte0           | Byte1       | Byte2          |
 *   | manufacturer ID | memory type | memory density |
 *   | 85              | 44          | 12             |
 *    ------------------------------------------------
 * FlashSize = 1UL << (memory density)
 *   0x10=64KB | 0x11=128KB | 0x12=256KB | 0x13=512KB
 ******************************************************/
static uint32_t msc_flash_size(void)
{
    // read FlashID
    uint8_t fshId[3];
    uint8_t fcmd = FSH_CMD_RD_ID;
    
    spi_flash_recv(&fcmd, 1, fshId, 3);

    // calc FlashSize
    return (fshId[2] > 0x20 ? 0 : (0x01UL << fshId[2]));
}

// Read Flash Data Aligned(4)
static void msc_flash_read(uint32_t offset, uint32_t *buff, uint32_t wlen)
{
    flash_op_t flash_op =
    {
        .op_cmd  = FSH_CMD_RD,
        .op_addr = offset,
    };

    flash_op.word = __REV(flash_op.word);

    DEBUG("rd of:%X, %X, %d", offset, flash_op.op_addr, CO_BLEN(wlen));
    spi_flash_recv(flash_op.op_data, 4, (uint8_t *)buff, CO_BLEN(wlen));
}

// Write Flash Data
void msc_flash_write(uint32_t offset, uint32_t *data, uint32_t wlen)
{
    uint8_t sta0;
    flash_op_t flash_op =
    {
        .op_cmd = FSH_CMD_WR,
        .op_addr = offset,
    };

    flash_op.word = __REV(flash_op.word);
    DEBUG("wr of:%X, %X, %d", offset, flash_op.op_addr, CO_BLEN(wlen));

    spi_flash_wren();
    spi_flash_send(flash_op.op_data, 4, (uint8_t *)data, CO_BLEN(wlen));

    do {
        sta0 = spi_flash_sta0_get();
    } while (WIP_BIT & sta0);
}

// Erase Flash Data
static void msc_flash_erase(uint32_t offset)
{
    uint8_t sta0;
    flash_op_t flash_op =
    {
        .op_cmd = FSH_CMD_ER_SECTOR,
        .op_addr = (offset & ~0xFFFUL),
    };

    flash_op.word = __REV(flash_op.word);
    DEBUG("erase of:%X, %X", offset, flash_op.op_addr);

    spi_flash_wren();
    spi_flash_send(flash_op.op_data, 4, NULL, 0);

    do {
        sta0 = spi_flash_sta0_get();
    } while (WIP_BIT & sta0);
}

#endif


/*
 * EXPORTED FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void usbd_msc_get_cap(uint8_t lun, uint32_t *block_num, uint32_t *block_size)
{
    msc_flash_init();

    *block_num  = msc_flash_size() / SECTOR_SIZE;
    *block_size = SECTOR_SIZE;
}

bool usbd_msc_sector_read(uint8_t lun, uint32_t sector, uint8_t *buffer, uint32_t length)
{
    DEBUG("rd lun:%d, sector:%X, len:%d, p:%X", lun, sector, length, (uint32_t)buffer);

    // read buffer
    msc_flash_read((sector * SECTOR_SIZE), (uint32_t *)buffer, CO_WLEN(length));

    return true;
}

bool usbd_msc_sector_write(uint8_t lun, uint32_t sector, const uint8_t *data, uint32_t length)
{
    uint32_t nb_page = PAGE_NB(length);
    DEBUG("wr lun:%X, %d, sector:%X, len:%d, nb:%d, p:%X", (uint32_t)rd_buff, sizeof(rd_buff), sector, length, nb_page, (uint32_t)data);

    msc_flash_read((sector * SECTOR_SIZE), rd_buff, CO_WLEN(length));

    if (xmemcmp(rd_buff, data, length))
    {
        msc_flash_erase(sector * SECTOR_SIZE);
    }

    // save data
    for (uint32_t page_idx = 0; page_idx < nb_page; ++page_idx)
    {
        msc_flash_write((sector * SECTOR_SIZE) + (page_idx * PAGE_SIZE), (uint32_t *)(data + (page_idx * PAGE_SIZE)), PAGE_WLEN);
    }

    return true;
}
