/**
 ****************************************************************************************
 *
 * @file ntag_test.c
 *
 * @brief NTAG I2C Plus test functions
 *
 * @details
 * NDEF record builders and test entry point for NTAG I2C Plus.
 * URI, Text, and combined URI+Text NDEF write operations.
 *
 ****************************************************************************************
 */

#include "ntag.h"
#include "drvs.h"
#include "dbg.h"
#include <string.h>


/*
 * DEFINES
 ****************************************************************************************
 */

/// NFC TLV / NDEF Constants
#define TLV_NDEF_MESSAGE        0x03
#define TLV_TERMINATOR          0xFE

#define NDEF_FLAG_MB_ME         0xD1
#define NDEF_FLAG_MB            0x91
#define NDEF_FLAG_ME            0x51

#define NDEF_TYPE_URI           0x55
#define NDEF_TYPE_TEXT          0x54

#define URI_EMPTY               0x00

/// TLV Buffer Size (user configurable)
#ifndef TLV_URI_SIZE
#define TLV_URI_SIZE            128
#endif

#ifndef TLV_TEXT_SIZE
#define TLV_TEXT_SIZE           128
#endif

#ifndef TLV_MULT_SIZE
#define TLV_MULT_SIZE           240
#endif


/*
 * NDEF BUILDERS
 ****************************************************************************************
 */

/**
 * @brief Build NDEF URI record
 *
 * @param[out] buf    Output buffer
 * @param[in]  nflag  NDEF record flags
 * @param[in]  prefix URI prefix type
 * @param[in]  size   URI string length
 * @param[in]  uri    URI string data
 * @return Total NDEF record length
 */
static inline uint8_t build_uri_ndef(uint8_t *buf, uint8_t nflag, uint8_t prefix,
                                      uint8_t size, const void *uri)
{
    buf[0] = nflag;
    buf[1] = 0x01;           // Type Size = 1
    buf[2] = 1 + size;       // Payload = 1(prefix) + n(uri_len)
    buf[3] = NDEF_TYPE_URI;
    buf[4] = prefix;
    memcpy(&buf[5], uri, size);
    return 5 + size;
}

/**
 * @brief Build NDEF Text record
 *
 * @param[out] buf    Output buffer
 * @param[in]  nflag  NDEF record flags
 * @param[in]  size   Text string length
 * @param[in]  text   Text string data
 * @return Total NDEF record length
 */
static inline uint8_t build_text_ndef(uint8_t *buf, uint8_t nflag, uint8_t size, const void *text)
{
    buf[0] = nflag;
    buf[1] = 0x01;           // Type Size = 1
    buf[2] = 1 + 2 + size;   // Payload = 1(encoding) + 2(lang) + n(text_len)
    buf[3] = NDEF_TYPE_TEXT;
    buf[4] = 0x02;           // UTF-8, 2-byte lang code
    buf[5] = 'e';
    buf[6] = 'n';
    memcpy(&buf[7], text, size);
    return 5 + 2 + size;
}


/*
 * TLV HELPERS
 ****************************************************************************************
 */

/**
 * @brief Wrap NDEF data in TLV structure and write to NTAG
 *
 * @param[in] start_block Start block address
 * @param[in] ndef_len    NDEF data length
 * @param[in] ndef_data   NDEF record data
 */
static ntag_status_t ntag_write_tlv(uint8_t start_block, uint8_t ndef_len, const uint8_t *ndef_data)
{
    uint8_t tlv_data[TLV_MULT_SIZE];

    if ((size_t)ndef_len + 3 > sizeof(tlv_data)) {
        return NTAG_ERR_INVALID_PARAM;
    }

    // TLV Header
    tlv_data[0] = TLV_NDEF_MESSAGE;
    tlv_data[1] = ndef_len;
    memcpy(&tlv_data[2], ndef_data, ndef_len);
    // TLV Terminator
    tlv_data[2 + ndef_len] = TLV_TERMINATOR;

    return ntag_write_bulk(start_block, ndef_len + 3, tlv_data);
}


/*
 * NDEF WRITE API
 ****************************************************************************************
 */

ntag_status_t ntag_write_uri(uint8_t start_block, uint8_t prefix, uint8_t size, const void *uri)
{
    if (size > TLV_URI_SIZE - 8) {
        return NTAG_ERR_INVALID_PARAM;
    }

    uint8_t ndef_data[TLV_URI_SIZE];
    uint8_t ndef_len = build_uri_ndef(ndef_data, NDEF_FLAG_MB_ME, prefix, size, uri);

    return ntag_write_tlv(start_block, ndef_len, ndef_data);
}

ntag_status_t ntag_write_text(uint8_t start_block, uint8_t size, const void *text)
{
    if (size > TLV_TEXT_SIZE - 10) {
        return NTAG_ERR_INVALID_PARAM;
    }

    uint8_t ndef_data[TLV_TEXT_SIZE];
    uint8_t ndef_len = build_text_ndef(ndef_data, NDEF_FLAG_MB_ME, size, text);

    return ntag_write_tlv(start_block, ndef_len, ndef_data);
}

ntag_status_t ntag_write_uri_text(uint8_t start_block, uint8_t prefix,
                                   uint8_t uri_len, const void *uri,
                                   uint8_t text_len, const void *text)
{
    if (uri_len + text_len > TLV_MULT_SIZE - (3 + 5 + 7)) {
        return NTAG_ERR_INVALID_PARAM;
    }

    uint8_t ndef_data[TLV_MULT_SIZE];
    uint8_t ndef_len = 0;

    ndef_len = build_uri_ndef(ndef_data, NDEF_FLAG_MB, prefix, uri_len, uri);
    ndef_len += build_text_ndef(&ndef_data[ndef_len], NDEF_FLAG_ME, text_len, text);

    return ntag_write_tlv(start_block, ndef_len, ndef_data);
}


/*
 * TEST
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief I2C NTAG test entry point
 ****************************************************************************************
 */
void i2c_test(void)
{
    ntag_status_t err = ntag_init();
    if (err != NTAG_OK) {
        debug("Init failed: %d\r\n", err);
        return;
    }

    const char *xdk = "1T2O0NQ9KSPV35I1X91WIOUV13N789GUTV3TO108QYGLUDM3B6F3HCNOFZW7LODH";

    debug("1. Writing XDK record...\r\n");
    #if (1)
    err = ntag_write_xdk(xdk, strlen(xdk));
    #else
    const char *uri = "X-DK://1T2O0NQ9KSPY35I1X91WIOUV13N789GUTV3TO108QYGLUDM3B6F3HCNOFZW7LODH";
    const char *text = "{\"DeviceType\":1,\"SN\":\"183BXN\",\"ConnectStatus\":0}";
    err = ntag_write_uri_text(1, URI_EMPTY, strlen(uri), uri, strlen(text), text);
    #endif
    if (err != NTAG_OK) {
        debug("Write XDK failed: %d\r\n", err);
    } else {
        debug("Write XDK: OK\r\n");
    }

    debug("2. Read Config Regs...\r\n");
    uint8_t block_data[16];
    err = ntag_read_block(0x3A, block_data);
    if (err == NTAG_OK) {
        debugHex(block_data, 8);
    } else {
        debug("Fail, err:%d\r\n", err);
    }

    debug("3. Read Session Regs...\r\n");
    uint8_t value = 0;
    for (int i = 0; i < 8; i++) {
        err = ntag_read_session(i, &value);
        debug("Reg[%d]=%02X, err:%d\r\n", i, value, err);
    }
}
