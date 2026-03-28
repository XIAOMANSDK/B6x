/**
 * @file ntag.c
 * @brief NTAG I2C plus
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

/* Field Dectect */
enum fd_trigger {
    FD_ON_FIELD = 0,    // RF field switch ON
    FD_ON_COMM_START,   // First valid start of Communication
    FD_ON_TAG_SELECTED, // Tag selected
    FD_ON_PASSTHROUGH   // Passthrough perpare
};

enum fd_release {
    FD_OFF_FIELD = 0,   // RF field switch OFF
    FD_OFF_HALT,        // Tag set to HALT
    FD_OFF_NDEF_READ,   // Read last page of NDEF msg
    FD_OFF_PASSTHROUGH  // Passthrough finish
};

/// NFC TLV Type
#define TLV_NDEF_MESSAGE        0x03
#define TLV_TERMINATOR          0xFE

/// TLV Buffer Size (user define)
#ifndef TLV_URI_SIZE
#define TLV_URI_SIZE            128
#endif

#ifndef TLV_TEXT_SIZE
#define TLV_TEXT_SIZE           128
#endif

#ifndef TLV_MULT_SIZE
#define TLV_MULT_SIZE           240
#endif

/// NFC NDEF Header
#define NDEF_TNF_EMPTY          0x00
#define NDEF_TNF_WELL_KNOWN     0x01
#define NDEF_TNF_MIME_MEDIA     0x02
#define NDEF_TNF_ABSOLUTE_URI   0x03
#define NDEF_TNF_EXTERNAL_TYPE  0x04
#define NDEF_TNF_UNKNOWN        0x05
#define NDEF_TNF_UNCHANGED      0x06
#define NDEF_TNF_RESERVED       0x07

#define NDEF_RECORD_FLAG_MB     (1 << 7) // Msg Begin
#define NDEF_RECORD_FLAG_ME     (1 << 6) // Msg End
#define NDEF_RECORD_FLAG_CF     (1 << 5) // Contain Flag
#define NDEF_RECORD_FLAG_SR     (1 << 4) // Short Record
#define NDEF_RECORD_FLAG_IL     (1 << 3) // ID Length exist

#define NDEF_FLAG_MB_ME         0xD1 //(FLAG_MB | FLAG_ME | FLAG_SR | TNF_WELL_KNOWN)
#define NDEF_FLAG_MB            0x91 //(FLAG_MB | FLAG_SR | TNF_WELL_KNOWN)
#define NDEF_FLAG_ME            0x51 //(FLAG_ME | FLAG_SR | TNF_WELL_KNOWN)

#define NDEF_TYPE_URI           0x55
#define NDEF_TYPE_TEXT          0x54

/// NDEF URI prefix type
typedef enum {
    URI_EMPTY           = 0x00, //""
    URI_HTTP_WWW        = 0x01, //"http://www."
    URI_HTTPS_WWW       = 0x02, //"https://www."
    URI_HTTP            = 0x03, //"http://"
    URI_HTTPS           = 0x04, //"https://"
    URI_TEL             = 0x05, //"tel:"
    URI_MAILTO          = 0x06, //"mailto:"
    URI_FTP_ANON        = 0x07, //"ftp://anonymous:anonymous@"
    URI_FTP_FTP         = 0x08, //"ftp://ftp."
    URI_FTPS            = 0x09, //"ftps://"
    URI_SFTP            = 0x0A, //"sftp://"
    URI_SMB             = 0x0B, //"smb://"
    URI_NFS             = 0x0C, //"nfs://"
    URI_FTP             = 0x0D, //"ftp://"
    URI_DAV             = 0x0E, //"dav://"
    URI_NEWS            = 0x0F, //"news:"
    URI_TELNET          = 0x10, //"telnet://"
    URI_IMAP            = 0x11, //"imap:"
    URI_RTSP            = 0x12, //"rtsp://"
    URI_URN             = 0x13, //"urn:"
    URI_POP             = 0x14, //"pop:"
    URI_SIP             = 0x15, //"sip:"
    URI_SIPS            = 0x16, //"sips:"
    URI_TFTP            = 0x17, //"tftp:"
    URI_BTSPP           = 0x18, //"btspp://"
    URI_BTL2CAP         = 0x19, //"btl2cap://"
    URI_BTGOEP          = 0x1A, //"btgoep://"
    URI_TCPOBEX         = 0x1B, //"tcpobex://"
    URI_IRDAOBEX        = 0x1C, //"irdaobex://"
    URI_FILE            = 0x1D, //"file://"
    URI_URN_EPC_ID      = 0x1E, //"urn:epc:id:"
    URI_URN_EPC_TAG     = 0x1F, //"urn:epc:tag:"
    URI_URN_EPC_PAT     = 0x20, //"urn:epc:pat:"
    URI_URN_EPC_RAW     = 0x21, //"urn:epc:raw:"
    URI_URN_EPC         = 0x22, //"urn:epc:"
    URI_URN_NFC         = 0x23, //"urn:nfc:"
} uri_type_t;


/*
 * DRIVER FUNCTION
 ****************************************************************************************
 */

/**
 * @brief Read block data(16 Bytes)
 *
 * @param[in]  block  block address(0x00~NTAG_MAX_BLOCK)
 * @param[out] data   block values
 */
static ntag_status_t ntag_read_block(uint8_t block, uint8_t *data)
{
    uint8_t status = i2c_start(NTAG_I2C_WRITE);
    if (status != I2C_MS_ADDR_W_ACK) {
        return NTAG_ERR_I2C_START;
    }

    /* try to write addr */
    if (i2c_send(1, &block) == 1) {
        i2c_stop();

        /* try to read block */
        status = i2c_start(NTAG_I2C_READ);
        if (status == I2C_MS_ADDR_R_ACK) {
            if (i2c_recv(16, data) == 16) {
                i2c_stop();
                return NTAG_OK;
            }
        }
    }

    /* Read Fail */
    i2c_stop();
    return NTAG_ERR_I2C_READ;
}

/**
 * @brief Write block data(16 Bytes)
 *
 * @param[in] block  block address(0x00~NTAG_MAX_BLOCK)
 * @param[in] data   data to write
 */
static ntag_status_t ntag_write_block(uint8_t block, const uint8_t *data)
{
    uint8_t status = i2c_start(NTAG_I2C_WRITE);
    if (status != I2C_MS_ADDR_W_ACK) {
        return NTAG_ERR_I2C_START;
    }

    /* try to write addr */
    if (i2c_send(1, &block) == 1) {
        /* try to write data */
        if (i2c_send(16, data) == 16) {
            i2c_stop();
            /* MUST wait ~4ms for EEPROM Programming */
            delay_ms(4);
            return NTAG_OK;
        }
    }

    /* Write Fail */
    i2c_stop();
    return NTAG_ERR_I2C_WRITE;
}

/**
 * @brief Write bulk data to blocks
 *
 * @param[in] block  start block(0x01~NTAG_MAX_BLOCK)
 * @param[in] size   length of data
 * @param[in] data   data to write
 */
static ntag_status_t ntag_write_bulk(uint8_t start_block, uint8_t size, const uint8_t *data)
{
    uint8_t block_data[16];
    uint8_t remaining = size;
    uint8_t offset = 0;
    uint8_t block = start_block;

    if (block + NTAG_SIZE2BLOCK(size) > NTAG_MAX_BLOCK) {
        return NTAG_ERR_INVALID_PARAM; // Exceeds user area
    }

    while (remaining > 0) {
        uint8_t to_write = (remaining > 16) ? 16 : remaining;

        memset(block_data, 0, 16);
        memcpy(block_data, data + offset, to_write);

        ntag_status_t status = ntag_write_block(block, block_data);
        if (status != NTAG_OK) return status;

        offset += to_write;
        remaining -= to_write;
        block++;
    }

    return NTAG_OK;
}

/**
 * @brief Read Session register value
 *
 * @param[in] n_addr   NTAG_CONFIG_BLOCK or NTAG_SESSION_ADDR
 * @param[in] reg_idx  register index
 * @param[out] value   register value
 */
static ntag_status_t ntag_read_session(uint8_t reg_idx, uint8_t *value)
{
    uint8_t rd_reg[2];
    uint8_t status = i2c_start(NTAG_I2C_WRITE);
    if (status != I2C_MS_ADDR_W_ACK) {
        return NTAG_ERR_I2C_START;
    }

    /* try to write rd_reg */
    rd_reg[0] = NTAG_SESSION_ADDR;
    rd_reg[1] = reg_idx;
    if (i2c_send(2, rd_reg) == 2) {
        i2c_stop();

        /* try to read value */
        status = i2c_start(NTAG_I2C_READ);
        if (status == I2C_MS_ADDR_R_ACK) {
            if (i2c_recv(1, value) == 1) {
                i2c_stop();
                return NTAG_OK;
            }
        }
    }

    /* Read Fail */
    i2c_stop();
    return NTAG_ERR_I2C_READ;
}

/**
 * @brief Write Session register value
 *
 * @param[in] n_addr   NTAG_CONFIG_BLOCK or NTAG_SESSION_ADDR
 * @param[in] reg_idx  register index
 * @param[in] value    register value
 * @param[in] mask     mask of value
 */
static ntag_status_t ntag_write_session(uint8_t reg_idx, uint8_t value, uint8_t mask)
{
    uint8_t wr_reg[4];
    uint8_t status = i2c_start(NTAG_I2C_WRITE);
    if (status != I2C_MS_ADDR_W_ACK) {
        return NTAG_ERR_I2C_START;
    }

    /* try to write wr_reg addr */
    wr_reg[0] = NTAG_SESSION_ADDR;
    wr_reg[1] = reg_idx;
    wr_reg[2] = mask;
    wr_reg[3] = value;
    if (i2c_send(4, wr_reg) == 4) {
        i2c_stop();
        return NTAG_OK;
    }

    /* Write Fail */
    i2c_stop();
    return NTAG_ERR_I2C_WRITE;
}


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

ntag_status_t ntag_init(void)
{
    ntag_on();
    delay_ms(2); // wait power-on
    i2c_init(NTAG_SCL_PIN, NTAG_SDA_PIN, NTAG_I2C_SCLK);

    DEBUG("Init NTAG...");

    // ntag_detect to read NTAG_CC_BLOCK(block0)
    uint8_t block_data[16];
    ntag_status_t err = ntag_read_block(NTAG_CC_BLOCK, block_data);
    if (err != NTAG_OK) {
        DEBUG("Init failed: device not responding");
        return err;
    }

    // verify NXP Manuf-ID(0x04)
    if (block_data[0] != 0x04) {
        DEBUG("Init failed: invalid manufacturer ID (expected 0x04, got 0x%02X)", block_data[0]);
        return NTAG_ERR_VERIFY_FAILED;
    }

    DEBUG("Current NTAG UID:"); debugHex(block_data, 7);

    // Init configure
    if (block_data[15] != 0x0F) {
        /* Init CC block as NFC RO */
        block_data[00] = 0xAA; // keep I2C Addr=0x55
        block_data[12] = 0xE1; // fixed Syncword
        block_data[13] = 0x10; // fixed Version
        block_data[14] = 0x6D; // size=(0x6D+1)*8
        block_data[15] = 0x0F; // NFC Read-Only
        err = ntag_write_block(NTAG_CC_BLOCK, block_data);
        DEBUG("Init CC: %d", err);
        /* Init Config block as FD output */
        err = ntag_read_block(NTAG_CONFIG_BLOCK, block_data);
        // 01 00 F8 48 08 01 00 00
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
    /* read NC_REG */
    uint8_t value;
    ntag_status_t err = ntag_read_session(NC_REG, &value);
    if (err != NTAG_OK){
        return err;
    }
    DEBUG("NFC Silent: %d", enable);

    /* check same */
    bool silent = ((value & NFC_SILENT_BIT) != 0);
    if (silent == enable) {
        return NTAG_OK;
    }

    /* modify SILENT_BIT of NC_REG */
    return ntag_write_session(NC_REG, enable * NFC_SILENT_BIT, NFC_SILENT_BIT);
}

/*
 prefix X-DK://
 Normal state: 1T2O0NQ9KSPV35I1X91WIOUV13N789GUTV3TO108QYGLUDM3B6F3HCNOFZW7LODH
 Invalid state: 2I5OPGCVANLS
 */
static const uint8_t XDK_INVALID[] = { '2','I','5','O','P','G','C','V','A','N','L','S' };

static inline uint8_t build_xdk_ndef(uint8_t *buf, uint8_t size, const void *xdk)
{
    /* NDEF URI Header */
    buf[0] = NDEF_FLAG_MB_ME;   // MB=1, ME=1, CF=0, SR=1, IL=0, TNF=1 (NFC Forum well-known type)
    buf[1] = 0x01;              // Type Size = 1
    buf[2] = 1 + 7 + size;      // Payl Size = 1(prefix) + 7("X-DK://") + n(xdk_len)
    buf[3] = NDEF_TYPE_URI;     // 'U' 0x55
    buf[4] = URI_EMPTY;         // URI prefix type
    /*NDEF URI Context */
    buf[5]  = 'X';
    buf[6]  = '-';
    buf[7]  = 'D';
    buf[8]  = 'K';
    buf[9]  = ':';
    buf[10] = '/';
    buf[11] = '/';
    memcpy(&buf[12], xdk, size);
    return 5 + 7 + size;
}

ntag_status_t ntag_write_xdk(const void *xdk, uint8_t xdk_len)
{
    uint8_t tlv_len;
    uint8_t tlv_data[80]; // 14+64=78 algin(16)

    if (xdk_len > 64) {
        return NTAG_ERR_INVALID_PARAM;
    }

    /* NULL to Invalid XDK */
    if (xdk == NULL) {
        xdk = XDK_INVALID;
        xdk_len = sizeof(XDK_INVALID);
    }

    /* NDEF msg */
    tlv_len = build_xdk_ndef(&tlv_data[2], xdk_len, xdk);
    /* TLV Header */
    tlv_data[0] = TLV_NDEF_MESSAGE;
    tlv_data[1] = tlv_len;
    tlv_len += 2;
    /* TLV Terminator */
    tlv_data[tlv_len++] = TLV_TERMINATOR;

    DEBUG("TLV structure: total_len=%d, ndef_len=%d", tlv_len, 1+7+xdk_len);
    return ntag_write_bulk(1, tlv_len, tlv_data);
}


#if (CFG_TEST)

/**
 * @brief Write URI NDEF
 */
static inline uint8_t build_uri_ndef(uint8_t *buf, uint8_t nflag, uint8_t prefix, uint8_t size, const void *uri)
{
    /* NDEF URI Header */
    buf[0] = nflag;         // MB=1, ME=1, CF=0, SR=1, IL=0, TNF=1
    buf[1] = 0x01;          // Type Size = 1
    buf[2] = 1 + size;      // Payl Size = 1(prefix) + n(text_len)
    buf[3] = NDEF_TYPE_URI; // 'U' 0x55
    buf[4] = prefix;        // URI prefix type
    /*NDEF URI Context */
    memcpy(&buf[5], uri, size);
    return 5 + size;
}

ntag_status_t ntag_write_uri(uint8_t start_block, uint8_t prefix, uint8_t size, const void *uri)
{
    if (size > TLV_URI_SIZE - 8) {
        return NTAG_ERR_INVALID_PARAM;
    }

    uint8_t tlv_len;
    uint8_t tlv_data[TLV_URI_SIZE];

    /* NDEF msg */
    tlv_len = build_uri_ndef(&tlv_data[2], NDEF_FLAG_MB_ME, prefix, size, uri);
    /* TLV Header */
    tlv_data[0] = TLV_NDEF_MESSAGE;
    tlv_data[1] = tlv_len;
    tlv_len += 2;
    /* TLV Terminator */
    tlv_data[tlv_len++] = TLV_TERMINATOR;

    return ntag_write_bulk(start_block, tlv_len, tlv_data);
}

/**
 * @brief Write Text NDEF
 */
static inline uint8_t build_text_ndef(uint8_t *buf, uint8_t nflag, uint8_t size, const void *text)
{
    /* NDEF Text Header */
    buf[0] = nflag;         // MB=1, ME=1, CF=0, SR=1, IL=0, TNF=1
    buf[1] = 0x01;          // Type Size = 1
    buf[2] = 1 + 2 + size;  // Payl Size = 1(encoding) + 2(lang_len) + n(text_len)
    buf[3] = NDEF_TYPE_TEXT;// 'T' 0x54
    /*NDEF Text Encoding */
    buf[4] = 0x02;          // encoding bit7:0=UTF-8,1=UTF-16;bit[6:0]:lang chars
    buf[5] = 'e';
    buf[6] = 'n';
    /*NDEF Text Context */
    memcpy(&buf[7], text, size);
    return 5 + 2 + size;
}

ntag_status_t ntag_write_text(uint8_t start_block, uint8_t size, const void *text)
{
    if (size > TLV_TEXT_SIZE - 10) {
        return NTAG_ERR_INVALID_PARAM;
    }

    uint8_t tlv_len;
    uint8_t tlv_data[TLV_TEXT_SIZE];

    /* NDEF msg */
    tlv_len = build_text_ndef(&tlv_data[2], NDEF_FLAG_MB_ME, size, text);
    /* TLV Header */
    tlv_data[0] = TLV_NDEF_MESSAGE;
    tlv_data[1] = tlv_len;
    tlv_len += 2;
    /* TLV Terminator */
    tlv_data[tlv_len++] = TLV_TERMINATOR;

    return ntag_write_bulk(start_block, tlv_len, tlv_data);
}

ntag_status_t ntag_write_uri_text(uint8_t start_block, uint8_t prefix, uint8_t uri_len, const void *uri,
                                  uint8_t text_len, const void *text)
{
    if (uri_len + text_len > TLV_MULT_SIZE - (3+5+7)) {
        return NTAG_ERR_INVALID_PARAM;
    }

    uint8_t tlv_len;
    uint8_t tlv_data[TLV_MULT_SIZE];

    /* NDEF msg */
    tlv_len = build_uri_ndef(&tlv_data[2], NDEF_FLAG_MB, prefix, uri_len, uri);
    tlv_len += build_text_ndef(&tlv_data[2 + tlv_len], NDEF_FLAG_ME, text_len, text);
    /* TLV Header */
    tlv_data[0] = TLV_NDEF_MESSAGE;
    tlv_data[1] = tlv_len;
    tlv_len += 2;
    /* TLV Terminator */
    tlv_data[tlv_len++] = TLV_TERMINATOR;

    return ntag_write_bulk(start_block, tlv_len, tlv_data);
}

#include <stdio.h>
const char* XDK = "1T2O0NQ9KSPV35I1X91WIOUV13N789GUTV3TO108QYGLUDM3B6F3HCNOFZW7LODH";

const char* URI = "X-DK://1T2O0NQ9KSPY35I1X91WIOUV13N789GUTV3TO108QYGLUDM3B6F3HCNOFZW7LODH";
const char* TEXT = "{\"DeviceType\":1,\"SN\":\"183BXN\",\"ConnectStatus\":0}";

void i2cTest(void)
{
    ntag_status_t err = ntag_init();
    if (err != NTAG_OK) {
        printf("Init failed: %d\n", err);
        return;
    }

    printf("1. Writing URI record...\n");
#if (1)
    err = ntag_write_xdk(XDK, strlen(XDK));
#else
    err = ntag_write_uri_text(1, URI_EMPTY, strlen(URI), URI, strlen(TEXT), TEXT);
#endif
    if (err != NTAG_OK) {
        printf("Write URI failed: %d\n", err);
    } else {
        printf("Write URI: OK\n");
    }

    printf("2. Read Config Regs...\n");
    uint8_t block_data[16];
    err = ntag_read_block(NTAG_CONFIG_BLOCK, block_data);
    if (err == NTAG_OK) {
        for (int i = 0; i < 8; i++) {
            printf("Reg[%d]=%02X\n", i, block_data[i]);
        }
    } else {
        printf("Fail, err:%d\n", err);
    }

    printf("3. Read Session Regs...\n");
    uint8_t value = 0;
    for (int i = 0; i < 8; i++) {
        err = ntag_read_session(i, &value);
        printf("Reg[%d]=%02X, err:%d\n", i, value, err);
    }
}
#endif
