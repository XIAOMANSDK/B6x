/**
 * @file ntag.h
 * @brief NTAG I2C Plus driver
 */

#ifndef NTAG_H
#define NTAG_H

#include <stdint.h>
#include <stdbool.h>

/*
 * STATUS
 ****************************************************************************************
 */

typedef enum {
    NTAG_OK = 0,
    NTAG_ERR_I2C_START,
    NTAG_ERR_I2C_WRITE,
    NTAG_ERR_I2C_READ,
    NTAG_ERR_INVALID_PARAM,
    NTAG_ERR_VERIFY_FAILED,
} ntag_status_t;

/*
 * NFC NDEF Constants
 ****************************************************************************************
 */

/// NFC TLV Type
#define TLV_NDEF_MESSAGE        0x03
#define TLV_TERMINATOR          0xFE

/// NDEF Record Flags
#define NDEF_RECORD_FLAG_MB     (1 << 7) // Msg Begin
#define NDEF_RECORD_FLAG_ME     (1 << 6) // Msg End
#define NDEF_RECORD_FLAG_CF     (1 << 5) // Contain Flag
#define NDEF_RECORD_FLAG_SR     (1 << 4) // Short Record
#define NDEF_RECORD_FLAG_IL     (1 << 3) // ID Length exist

#define NDEF_FLAG_MB_ME         0xD1 // MB+ME+SR+TNF_WELL_KNOWN
#define NDEF_FLAG_MB            0x91 // MB+SR+TNF_WELL_KNOWN
#define NDEF_FLAG_ME            0x51 // ME+SR+TNF_WELL_KNOWN

#define NDEF_TYPE_URI           0x55
#define NDEF_TYPE_TEXT          0x54

/// NDEF URI prefix type
typedef enum {
    URI_EMPTY           = 0x00,
    URI_HTTP_WWW        = 0x01,
    URI_HTTPS_WWW       = 0x02,
    URI_HTTP            = 0x03,
    URI_HTTPS           = 0x04,
    URI_TEL             = 0x05,
    URI_MAILTO          = 0x06,
} uri_type_t;

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
 * API
 ****************************************************************************************
 */

ntag_status_t ntag_init(void);
void ntag_deinit(void);
ntag_status_t ntag_nfc_silent(bool enable);

/**
 * @brief Read block data (16 bytes)
 *
 * @param[in]  block  Block address (0x00~0x3A)
 * @param[out] data   Block values (16 bytes)
 */
ntag_status_t ntag_read_block(uint8_t block, uint8_t *data);

/**
 * @brief Read session register value
 *
 * @param[in]  reg_idx  Register index
 * @param[out] value    Register value
 */
ntag_status_t ntag_read_session(uint8_t reg_idx, uint8_t *value);

/**
 * @brief Write bulk data to blocks
 *
 * @param[in] start_block  Start block (0x01~NTAG_MAX_BLOCK)
 * @param[in] size         Length of data
 * @param[in] data         Data to write
 */
ntag_status_t ntag_write_bulk(uint8_t start_block, uint8_t size, const uint8_t *data);

/**
 * @brief Write X-DK:// URI NDEF to NTAG
 *
 * @param[in] xdk     XDK string (NULL for invalid marker)
 * @param[in] xdk_len Length of XDK string (max 64)
 */
ntag_status_t ntag_write_xdk(const void *xdk, uint8_t xdk_len);

#endif
