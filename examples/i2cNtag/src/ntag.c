/**
 ****************************************************************************************
 *
 * @file ntag.c
 *
 * @brief NTAG I2C Plus driver implementation
 *
 * @details
 * Low-level driver for NTAG I2C Plus NFC tag (NT3H2111/2211).
 * Supports block read/write, session register access, and NDEF X-DK write.
 *
 ****************************************************************************************
 */

#include "ntag.h"
#include "i2c.h"
#include "drvs.h"
#include <string.h>

#if (DBG_NTAG)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, (int)__LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif


/*
 * DEFINES
 ****************************************************************************************
 */

/// NTAG Pin Defines
#define NTAG_I2C_SCLK           SCLK_200K
#define NTAG_SCL_PIN            PA10
#define NTAG_SDA_PIN            PA11
#define NTAG_FD_PIN             PA12
#define NTAG_VCC_PIN            PA13

/// Macro Functions - drvs.h
#define ntag_on()               GPIO_DAT_SET(BIT(NTAG_VCC_PIN))
#define ntag_off()              GPIO_DAT_CLR(BIT(NTAG_VCC_PIN))
#define delay_ms(tm)            bootDelayMs(tm)

/// NTAG Spec Defines
#define NTAG_I2C_ADDR           0x55 // 7bits addr
#define NTAG_I2C_WRITE          0xAA // (0x55<<1)|0
#define NTAG_I2C_READ           0xAB // (0x55<<1)|1

#define NTAG_EEPROM_SIZE        1024
#define NTAG_SRAM_SIZE          64
#define NTAG_BLOCK_SIZE         16
#define NTAG_SIZE2BLOCK(size)   (((size) + NTAG_BLOCK_SIZE - 1) / NTAG_BLOCK_SIZE)

#define NTAG_CC_BLOCK           0x00  // CC in Block0[12:15]
#define NTAG_MAX_BLOCK          0x37
#define NTAG_CONFIG_BLOCK       0x3A
#define NTAG_SESSION_ADDR       0xFE
#define NTAG_SRAM_BLOCK         0xF8

/* Register Index */
enum ntag_reg_idx {
    NC_REG              = 0,
    LAST_NDEF_BLOCK     = 1,
    SRAM_MIRROR_BLOCK   = 2,
    WDT_LS              = 3,
    WDT_MS              = 4,
    I2C_CLOCK_STR       = 5,
    REG_LOCK            = 6, // Config
    NS_LOCK             = 6, // Session
    RFU                 = 7,
};

/* NC_REG Bits-field */
enum nc_reg_bfs {
    NFC_SILENT_BIT      = (1 << 7),
    PTHRU_ON_OFF_BIT    = (1 << 6),

    FD_OFF_LSB          = 4, // bit[5:4]
    FD_OFF_MASK         = 0x30,
    FD_ON_LSB           = 2, // bit[3:2]
    FD_ON_MASK          = 0x0C,

    SRAM_MIRROR_BIT     = (1 << 1),
    TRANSFER_DIR_BIT    = (1 << 0),
};

/* Field Detect */
enum fd_trigger {
    FD_ON_FIELD = 0,
    FD_ON_COMM_START,
    FD_ON_TAG_SELECTED,
    FD_ON_PASSTHROUGH
};

enum fd_release {
    FD_OFF_FIELD = 0,
    FD_OFF_HALT,
    FD_OFF_NDEF_READ,
    FD_OFF_PASSTHROUGH
};

/// NFC TLV / NDEF Constants
#define TLV_NDEF_MESSAGE        0x03
#define TLV_TERMINATOR          0xFE

#define NDEF_FLAG_MB_ME         0xD1
#define NDEF_FLAG_MB            0x91
#define NDEF_FLAG_ME            0x51

#define NDEF_TYPE_URI           0x55
#define NDEF_TYPE_TEXT          0x54

#define URI_EMPTY               0x00


/*
 * DRIVER FUNCTIONS
 ****************************************************************************************
 */

/**
 * @brief Read block data (16 bytes)
 *
 * @param[in]  block  Block address (0x00~NTAG_MAX_BLOCK)
 * @param[out] data   Block values (16 bytes)
 */
ntag_status_t ntag_read_block(uint8_t block, uint8_t *data)
{
    uint8_t status = i2c_start(NTAG_I2C_WRITE);
    if (status != I2C_MS_ADDR_W_ACK) {
        return NTAG_ERR_I2C_START;
    }

    if (i2c_send(1, &block) == 1) {
        i2c_stop();

        status = i2c_start(NTAG_I2C_READ);
        if (status == I2C_MS_ADDR_R_ACK) {
            if (i2c_recv(16, data) == 16) {
                i2c_stop();
                return NTAG_OK;
            }
        }
    }

    i2c_stop();
    return NTAG_ERR_I2C_READ;
}

/**
 * @brief Write block data (16 bytes)
 *
 * @param[in] block  Block address (0x00~NTAG_MAX_BLOCK)
 * @param[in] data   Data to write (16 bytes)
 */
static ntag_status_t ntag_write_block(uint8_t block, const uint8_t *data)
{
    uint8_t status = i2c_start(NTAG_I2C_WRITE);
    if (status != I2C_MS_ADDR_W_ACK) {
        return NTAG_ERR_I2C_START;
    }

    if (i2c_send(1, &block) == 1) {
        if (i2c_send(16, data) == 16) {
            i2c_stop();
            // MUST wait ~4ms for EEPROM Programming
            delay_ms(4);
            return NTAG_OK;
        }
    }

    i2c_stop();
    return NTAG_ERR_I2C_WRITE;
}

/**
 * @brief Write bulk data to blocks
 *
 * @param[in] start_block  Start block (0x01~NTAG_MAX_BLOCK)
 * @param[in] size         Length of data
 * @param[in] data         Data to write
 */
ntag_status_t ntag_write_bulk(uint8_t start_block, uint8_t size, const uint8_t *data)
{
    uint8_t block = start_block;
    uint8_t remaining = size;
    uint8_t offset = 0;

    if (block + NTAG_SIZE2BLOCK(size) > NTAG_MAX_BLOCK) {
        return NTAG_ERR_INVALID_PARAM;
    }

    while (remaining > 0) {
        ntag_status_t status;

        if (remaining >= NTAG_BLOCK_SIZE) {
            // Full block: send directly from source, no copy
            status = ntag_write_block(block, data + offset);
            offset += NTAG_BLOCK_SIZE;
            remaining -= NTAG_BLOCK_SIZE;
        } else {
            // Partial last block: zero-pad via local buffer
            uint8_t block_data[NTAG_BLOCK_SIZE];
            memset(block_data, 0, NTAG_BLOCK_SIZE);
            memcpy(block_data, data + offset, remaining);
            status = ntag_write_block(block, block_data);
            remaining = 0;
        }

        if (status != NTAG_OK) {
            return status;
        }
        block++;
    }

    return NTAG_OK;
}

/**
 * @brief Read session register value
 *
 * @param[in]  reg_idx  Register index
 * @param[out] value    Register value
 */
ntag_status_t ntag_read_session(uint8_t reg_idx, uint8_t *value)
{
    uint8_t rd_reg[2];
    uint8_t status = i2c_start(NTAG_I2C_WRITE);
    if (status != I2C_MS_ADDR_W_ACK) {
        return NTAG_ERR_I2C_START;
    }

    rd_reg[0] = NTAG_SESSION_ADDR;
    rd_reg[1] = reg_idx;
    if (i2c_send(2, rd_reg) == 2) {
        i2c_stop();

        status = i2c_start(NTAG_I2C_READ);
        if (status == I2C_MS_ADDR_R_ACK) {
            if (i2c_recv(1, value) == 1) {
                i2c_stop();
                return NTAG_OK;
            }
        }
    }

    i2c_stop();
    return NTAG_ERR_I2C_READ;
}

/**
 * @brief Write session register value
 *
 * @param[in] reg_idx  Register index
 * @param[in] value    Register value
 * @param[in] mask     Value mask
 */
static ntag_status_t ntag_write_session(uint8_t reg_idx, uint8_t value, uint8_t mask)
{
    uint8_t wr_reg[4];
    uint8_t status = i2c_start(NTAG_I2C_WRITE);
    if (status != I2C_MS_ADDR_W_ACK) {
        return NTAG_ERR_I2C_START;
    }

    wr_reg[0] = NTAG_SESSION_ADDR;
    wr_reg[1] = reg_idx;
    wr_reg[2] = mask;
    wr_reg[3] = value;
    if (i2c_send(4, wr_reg) == 4) {
        i2c_stop();
        return NTAG_OK;
    }

    i2c_stop();
    return NTAG_ERR_I2C_WRITE;
}


/*
 * PUBLIC API
 ****************************************************************************************
 */

ntag_status_t ntag_init(void)
{
    ntag_on();
    delay_ms(2); // Wait power-on
    i2c_init(NTAG_SCL_PIN, NTAG_SDA_PIN, NTAG_I2C_SCLK);

    DEBUG("Init NTAG...");

    // Read CC block (block 0) to detect NTAG
    uint8_t block_data[16];
    ntag_status_t err = ntag_read_block(NTAG_CC_BLOCK, block_data);
    if (err != NTAG_OK) {
        DEBUG("Init failed: device not responding");
        return err;
    }

    // Verify NXP Manuf-ID (0x04)
    if (block_data[0] != 0x04) {
        DEBUG("Init failed: invalid manufacturer ID (expected 0x04, got 0x%02X)", block_data[0]);
        return NTAG_ERR_VERIFY_FAILED;
    }

    DEBUG("Current NTAG UID:"); debugHex(block_data, 7);

    // Init configuration if needed
    if (block_data[15] != 0x0F) {
        // Init CC block as NFC Read-Only
        block_data[0]  = 0xAA; // Keep I2C Addr=0x55
        block_data[12] = 0xE1; // Fixed syncword
        block_data[13] = 0x10; // Fixed version
        block_data[14] = 0x6D; // Size=(0x6D+1)*8
        block_data[15] = 0x0F; // NFC Read-Only
        err = ntag_write_block(NTAG_CC_BLOCK, block_data);
        DEBUG("Init CC: %d", err);

        // Init Config block for FD output
        err = ntag_read_block(NTAG_CONFIG_BLOCK, block_data);
        DEBUG("Read Config: %d", err); debugHex(block_data, 8);

        // Set FD_ON/FD_OFF in Config & Session
        uint8_t nc_reg = (FD_OFF_FIELD << FD_OFF_LSB) | (FD_ON_TAG_SELECTED << FD_ON_LSB) | 0x01;
        block_data[NC_REG] = nc_reg;
        err = ntag_write_block(NTAG_CONFIG_BLOCK, block_data);
        DEBUG("Write Config: %d", err);
        err = ntag_write_session(NC_REG, nc_reg, 0xFF);
        DEBUG("Write Session: %d", err);
    }

    return NTAG_OK;
}

void ntag_deinit(void)
{
    i2c_deinit();
    ntag_off();
}

ntag_status_t ntag_nfc_silent(bool enable)
{
    uint8_t value;
    ntag_status_t err = ntag_read_session(NC_REG, &value);
    if (err != NTAG_OK) {
        return err;
    }
    DEBUG("NFC Silent: %d", enable);

    bool silent = ((value & NFC_SILENT_BIT) != 0);
    if (silent == enable) {
        return NTAG_OK;
    }

    return ntag_write_session(NC_REG, enable * NFC_SILENT_BIT, NFC_SILENT_BIT);
}

/**
 ****************************************************************************************
 * @brief Build X-DK:// NDEF URI record
 *
 * @param[out] buf    Output buffer
 * @param[in]  size   XDK string length
 * @param[in]  xdk    XDK string data
 * @return Total NDEF record length
 ****************************************************************************************
 */
static inline uint8_t build_xdk_ndef(uint8_t *buf, uint8_t size, const void *xdk)
{
    buf[0] = NDEF_FLAG_MB_ME;
    buf[1] = 0x01;               // Type Size = 1
    buf[2] = 1 + 7 + size;       // Payload = 1(prefix) + 7("X-DK://") + n(xdk_len)
    buf[3] = NDEF_TYPE_URI;
    buf[4] = URI_EMPTY;

    memcpy(&buf[5], "X-DK://", 7);
    memcpy(&buf[12], xdk, size);
    return 5 + 7 + size;
}

ntag_status_t ntag_write_xdk(const void *xdk, uint8_t xdk_len)
{
    uint8_t tlv_len;
    uint8_t tlv_data[80]; // 14+64=78 align(16)

    if (xdk_len > 64) {
        return NTAG_ERR_INVALID_PARAM;
    }

    static const uint8_t xdk_invalid[] = { '2','I','5','O','P','G','C','V','A','N','L','S' };

    // NULL maps to invalid XDK marker
    if (xdk == NULL) {
        xdk = xdk_invalid;
        xdk_len = sizeof(xdk_invalid);
    }

    // Build NDEF message
    tlv_len = build_xdk_ndef(&tlv_data[2], xdk_len, xdk);
    // TLV Header
    tlv_data[0] = TLV_NDEF_MESSAGE;
    tlv_data[1] = tlv_len;
    tlv_len += 2;
    // TLV Terminator
    tlv_data[tlv_len++] = TLV_TERMINATOR;

    DEBUG("TLV structure: total_len=%d, ndef_len=%d", tlv_len, 1 + 7 + xdk_len);
    return ntag_write_bulk(1, tlv_len, tlv_data);
}
