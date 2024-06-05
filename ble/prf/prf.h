/**
 ****************************************************************************************
 *
 * @file prf.h
 *
 * @brief Header file - used to manage and access BLE profiles.
 *
 ****************************************************************************************
 */

#ifndef _PRF_H_
#define _PRF_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include <stdint.h>
#include <stdbool.h>
#include "string.h"

#include "blelib.h"
#include "le_err.h"
#include "att.h"
#include "attm_api.h"
#include "gatt_api.h"
#include "gapc_api.h"


/*
 * DEFINES
 ****************************************************************************************
 */

/// Attribute is mandatory
#define ATT_MANDATORY           (0xFF)
/// Attribute is optional
#define ATT_OPTIONAL            (0x00)

/// Possible values for setting client configuration characteristics
enum prf_cli_conf
{
    /// Stop notification/indication
    PRF_CLI_STOP_NTFIND         = 0x0000,
    /// Start notification
    PRF_CLI_START_NTF           = 0x0001,
    /// Start indication
    PRF_CLI_START_IND           = 0x0002,
};

/// Possible values for setting server configuration characteristics
enum prf_srv_conf
{
    /// Stop Broadcast
    PRF_SRV_STOP_BCST           = 0x0000,
    /// Start Broadcast
    PRF_SRV_START_BCST          = 0x0001,
};

/// Service type
enum prf_svc_type
{
    PRF_PRIMARY_SERVICE         = 0x00,
    PRF_SECONDARY_SERVICE       = 0x01
};


/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Characteristic Presentation Format Descriptor Size
#define PRF_PRES_FMT_SIZE       (7)

/// Characteristic Presentation information
struct prf_pres_fmt
{
    /// Unit (The Unit is a UUID)
    uint16_t unit;
    /// Description
    uint16_t description;
    /// Format
    uint8_t format;
    /// Exponent
    uint8_t exponent;
    /// Name space
    uint8_t name_space;
};

/// Time profile Date and Time Size
#define PRF_DATA_TIME_SIZE      (7)

/// Time profile information
struct prf_date_time
{
    /// year time element
    uint16_t year;
    /// month time element
    uint8_t month;
    /// day time element
    uint8_t day;
    /// hour time element
    uint8_t hour;
    /// minute time element
    uint8_t min;
    /// second time element
    uint8_t sec;
};

/**
 *  SFLOAT: Short Floating Point Type
 *
 *        +----------+----------+---------+
 *        | Exponent | Mantissa |  Total  |
 * +------+----------+----------+---------+
 * | size |  4 bits  | 12 bits  | 16 bits |
 * +------+----------+----------+---------+
 */
typedef uint16_t prf_sfloat;

/// Attribute information
struct prf_att_info
{
    /// Attribute Handle
    uint16_t handle;
    /// Attribute length
    uint16_t length;
    /// Status of request
    uint8_t  status;
    /// Attribute value
    uint8_t value[];
};

/// service handles
struct prf_svc
{
    /// start handle
    uint16_t shdl;
    /// end handle
    uint16_t ehdl;
};

/// included service handles
struct prf_incl_svc
{
    /// attribute handle
    uint16_t handle;
    /// included service start handle
    uint16_t start_hdl;
    /// included service  end handle
    uint16_t end_hdl;
    /// UUID length
    uint8_t uuid_len;
    /// UUID
    uint8_t uuid[ATT_UUID128_LEN];
};

/// characteristic info
struct prf_char_inf
{
    /// Characteristic handle
    uint16_t char_hdl;
    /// Value handle
    uint16_t val_hdl;
    /// Characteristic properties
    uint8_t prop;
    /// End of characteristic offset
    uint8_t char_ehdl_off;
};

/// characteristic description
struct prf_desc_inf
{
    /// Descriptor handle
    uint16_t desc_hdl;
};

/// Characteristic definition
struct prf_char_def
{
    /// Characteristic UUID
    uint16_t uuid;
    /// Requirement Attribute Flag
    uint8_t req_flag;
    /// Mandatory Properties
    uint8_t prop_mand;
};

/// Characteristic Descriptor definition
struct prf_desc_def
{
    /// Characteristic Descriptor uuid
    uint16_t uuid;
    /// requirement attribute flag
    uint8_t req_flag;
    /// Corresponding characteristic code
    uint8_t char_code;
};

/// Message structure used to inform APP that a profile client role has been disabled
struct prf_client_disable_ind
{
    /// Status
    uint8_t status;
};

/// Message structure used to inform APP that an error has occured in the profile server role task
struct prf_server_error_ind
{
    /// Message ID
    uint16_t msg_id;
    /// Status
    uint8_t status;
};


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief SVR Pack Characteristic Presentation Format descriptor value
 ****************************************************************************************
 */
void prf_pack_pres_fmt(uint8_t *packed_val, const struct prf_pres_fmt* pres_fmt);

/**
 ****************************************************************************************
 * @brief SVR Pack date time value
 *
 * @param[out] packed_date packed date time
 * @param[in] date_time structure date time
 *
 * @return size of packed value
 ****************************************************************************************
 */
uint8_t prf_pack_date_time(uint8_t *packed_date, const struct prf_date_time* date_time);


/**
 ****************************************************************************************
 * @brief CLI Unpack Characteristic Presentation Format descriptor value
 ****************************************************************************************
 */
void prf_unpack_pres_fmt(const uint8_t *packed_val, struct prf_pres_fmt* pres_fmt);

/**
 ****************************************************************************************
 * @brief CLI Unpack date time value
 *
 * @param[in] packed_date packed date time
 * @param[out] date_time structure date time
 *
 * @return size of packed value
 ****************************************************************************************
 */
uint8_t prf_unpack_date_time(uint8_t *packed_date, struct prf_date_time* date_time);


#endif /* _PRF_H_ */
