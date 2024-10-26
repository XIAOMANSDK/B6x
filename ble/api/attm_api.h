/**
 ****************************************************************************************
 *
 * @file attm_api.h
 *
 * @brief Attribute Manager API functions.
 *
 ****************************************************************************************
 */

#ifndef ATTM_API_H_
#define ATTM_API_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include <stdint.h>
#include <stdbool.h>
#include "att.h"
#include "task.h"


/*
 * MACRO DEFINES
 ****************************************************************************************
 */

/// update attribute permission on specific handle
#define ATTM_UPDATE_PERM(handle, access, right)    \
    attm_att_update_perm(handle, (PERM_MASK_ ## access), PERM(access, right))

#define ATTM_UPDATE_PERM_VAL(handle, access, val)  \
    attm_att_update_perm(handle, (PERM_MASK_ ## access), ((val) << (PERM_POS_ ## access)))

// --------------------------- Database permissions -----------------------------

/// Macro used to retrieve access permission rights
#define PERM_GET(perm, access)\
        (((perm) & (PERM_MASK_ ## access)) >> (PERM_POS_ ## access))

/// Macro used to set a permission value
#define PERM_SET(perm, access, value)\
    perm = ((perm & ~(PERM_MASK_ ## access)) | ((value << (PERM_POS_ ## access)) & (PERM_MASK_ ## access)))

/// Macro used to retrieve permission value from access and rights on attribute.
#define PERM(access, right) \
    (((PERM_RIGHT_ ## right) << (PERM_POS_ ## access)) & (PERM_MASK_ ## access))

/// Macro used know if permission is set or not.
#define PERM_IS_SET(perm, access, right) \
    (((perm) & (((PERM_RIGHT_ ## right) << (PERM_POS_ ## access))) \
                & (PERM_MASK_ ## access)) == PERM(access, right))

/// Macro used to create permission value
#define PERM_VAL(access, perm) \
    ((((perm) << (PERM_POS_ ## access))) & (PERM_MASK_ ## access))


/// Retrieve attribute security level from attribute right and service right
#define ATT_GET_SEC_LVL(att_right, svc_right) \
    co_max(((att_right) & PERM_RIGHT_AUTH), ((svc_right) & PERM_RIGHT_AUTH));

/// Retrieve UUID LEN from UUID Length Permission
#define ATT_UUID_LEN(uuid_len_perm) ((uuid_len_perm == 0) ? ATT_UUID16_LEN : \
        ((uuid_len_perm == 1) ? ATT_UUID32_LEN  :                        \
        ((uuid_len_perm == 2) ? ATT_UUID128_LEN : 0)))

/// Initialization of attribute element
#define ATT_ELEMT_INIT                                   {{NULL}, false}

/**
 *   15   14   13   12   11   10    9    8    7    6    5    4    3    2    1    0
 * +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
 * |EXT | WS | I  | N  | WR | WC | RD | B  |    NP   |    IP   |   WP    |    RP   |
 * +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
 *
 * Bit [0-1]  : Read Permission         (0 = NO_AUTH, 1 = UNAUTH, 2 = AUTH, 3 = SEC_CON)
 * Bit [2-3]  : Write Permission        (0 = NO_AUTH, 1 = UNAUTH, 2 = AUTH, 3 = SEC_CON)
 * Bit [4-5]  : Indication Permission   (0 = NO_AUTH, 1 = UNAUTH, 2 = AUTH, 3 = SEC_CON)
 * Bit [6-7]  : Notification Permission (0 = NO_AUTH, 1 = UNAUTH, 2 = AUTH, 3 = SEC_CON)
 *
 * Bit [8]    : Extended properties present (only relevant for a characteristic value)
 * Bit [9]    : Broadcast permission        (only relevant for a characteristic value)
 * Bit [10]   : Write Command accepted
 * Bit [11]   : Write Signed accepted
 * Bit [12]   : Write Request accepted
 * Bit [13]   : Encryption key Size must be 16 bytes
 */
enum attm_perm_mask
{
    /// retrieve all permission info
    PERM_MASK_ALL           = 0x0000,
    /// Read Permission Mask
    PERM_MASK_RP            = 0x0003,
    PERM_POS_RP             = 0,
    /// Write Permission Mask
    PERM_MASK_WP            = 0x000C,
    PERM_POS_WP             = 2,
    /// Indication Access Mask
    PERM_MASK_IP            = 0x0030,
    PERM_POS_IP             = 4,
    /// Notification Access Mask
    PERM_MASK_NP            = 0x00C0,
    PERM_POS_NP             = 6,
    /// Broadcast descriptor present
    PERM_MASK_BROADCAST     = 0x0100,
    PERM_POS_BROADCAST      = 8,
    /// Read Access Mask
    PERM_MASK_RD            = 0x0200,
    PERM_POS_RD             = 9,
    /// Write Command Enabled attribute Mask
    PERM_MASK_WRITE_COMMAND = 0x0400,
    PERM_POS_WRITE_COMMAND  = 10,
    /// Write Request Enabled attribute Mask
    PERM_MASK_WRITE_REQ     = 0x0800,
    PERM_POS_WRITE_REQ      = 11,
    /// Notification Access Mask
    PERM_MASK_NTF           = 0x1000,
    PERM_POS_NTF            = 12,
    /// Indication Access Mask
    PERM_MASK_IND           = 0x2000,
    PERM_POS_IND            = 13,
    /// Write Signed Enabled attribute Mask
    PERM_MASK_WRITE_SIGNED  = 0x4000,
    PERM_POS_WRITE_SIGNED   = 14,
    /// Extended properties descriptor present
    PERM_MASK_EXT           = 0x8000,
    PERM_POS_EXT            = 15,

    /// Properties
    PERM_MASK_PROP          = 0xFF00,
    PERM_POS_PROP           = 8,
};

/**
 * Value permission bit field
 *
 *   15   14   13   12   11   10    9    8    7    6    5    4    3    2    1    0
 * +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
 * | RI |UUID_LEN |EKS |    MAX_LEN (RI = 1) / Value Offset  (RI = 0)              |
 * +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
 *
 * Bit [0-11] : Maximum Attribute Length or Value Offset pointer
 * Bit [12]   : Encryption key Size must be 16 bytes
 * Bit [14-13]: UUID Length             (0 = 16 bits, 1 = 32 bits, 2 = 128 bits, 3 = RFU)
 * Bit [15]   : Trigger Read Indication (0 = Value present in Database, 1 = Value not present in Database)
 */
enum attm_value_perm_mask
{
    /// Maximum Attribute Length
    PERM_MASK_MAX_LEN     = 0x0FFF,
    PERM_POS_MAX_LEN      = 0,
    /// Attribute value Offset
    PERM_MASK_VAL_OFFSET  = 0x0FFF,
    PERM_POS_VAL_OFFSET   = 0,
    /// Check Encryption key size Mask
    PERM_MASK_EKS         = 0x1000,
    PERM_POS_EKS          = 12,
    /// UUID Length
    PERM_MASK_UUID_LEN    = 0x6000,
    PERM_POS_UUID_LEN     = 13,
    /// Read trigger Indication
    PERM_MASK_RI          = 0x8000,
    PERM_POS_RI           = 15,
};


/**
 * Service permissions
 *
 *    7    6    5    4    3    2    1    0
 * +----+----+----+----+----+----+----+----+
 * |SEC |UUID_LEN |DIS |  AUTH   |EKS | MI |
 * +----+----+----+----+----+----+----+----+
 *
 * Bit [0]  : Task that manage service is multi-instantiated (Connection index is conveyed)
 * Bit [1]  : Encryption key Size must be 16 bytes
 * Bit [2-3]: Service Permission      (0 = NO_AUTH, 1 = UNAUTH, 2 = AUTH, 3 = Secure Connect)
 * Bit [4]  : Disable the service
 * Bit [5-6]: UUID Length             (0 = 16 bits, 1 = 32 bits, 2 = 128 bits, 3 = RFU)
 * Bit [7]  : Secondary Service       (0 = Primary Service, 1 = Secondary Service)
 */
enum attm_svc_perm_mask
{
    /// Task that manage service is multi-instantiated
    PERM_MASK_SVC_MI        = 0x01,
    PERM_POS_SVC_MI         = 0,
    /// Check Encryption key size for service Access
    PERM_MASK_SVC_EKS       = 0x02,
    PERM_POS_SVC_EKS        = 1,
    /// Service Permission authentication
    PERM_MASK_SVC_AUTH      = 0x0C,
    PERM_POS_SVC_AUTH       = 2,
    /// Disable the service
    PERM_MASK_SVC_DIS       = 0x10,
    PERM_POS_SVC_DIS        = 4,
    /// Service UUID Length
    PERM_MASK_SVC_UUID_LEN  = 0x60,
    PERM_POS_SVC_UUID_LEN   = 5,
    /// Service type Secondary
    PERM_MASK_SVC_SECONDARY = 0x80,
    PERM_POS_SVC_SECONDARY  = 7,
};

/// Attribute & Service access mode
enum
{
    /// Disable access
    PERM_RIGHT_DISABLE  = 0,
    /// Enable access
    PERM_RIGHT_ENABLE   = 1,
};

/// Attribute & Service access rights
enum
{
    /// No Authentication
    PERM_RIGHT_NO_AUTH  = 0,
    /// Access Requires Unauthenticated link
    PERM_RIGHT_UNAUTH   = 1,
    /// Access Requires Authenticated link
    PERM_RIGHT_AUTH     = 2,
    /// Access Requires Secure Connection link
    PERM_RIGHT_SEC_CON  = 3,
};

/// Attribute & Service UUID Length
enum
{
    /// 16  bits UUID
    PERM_UUID_16         = 0,
    PERM_RIGHT_UUID_16   = 0,
    /// 32  bits UUID
    PERM_UUID_32         = 1,
    PERM_RIGHT_UUID_32   = 1,
    /// 128 bits UUID
    PERM_UUID_128        = 2,
    PERM_RIGHT_UUID_128  = 2,
    /// Invalid
    PERM_UUID_RFU        = 3,
};

/// execute flags
enum
{
    /// Cancel All the Reliable Writes
    ATT_CANCEL_ALL_PREPARED_WRITES = 0x00,
    /// Write All the Reliable Writes
    ATT_EXECUTE_ALL_PREPARED_WRITES
};

/*
 * DATA STRUCTURES
 ****************************************************************************************
 */

/// 16bits UUID service description
struct attm_desc
{
    /// 16 bits UUID LSB First
    uint16_t uuid;
    /// Attribute Permissions (@see enum attm_perm_mask)
    uint16_t perm;
    /// Attribute Extended Permissions (@see enum attm_value_perm_mask)
    uint16_t ext_perm;
    /// Attribute Max Size
    /// note: for characteristic declaration contains handle offset
    /// note: for included service, contains target service handle
    uint16_t max_size;
};


/// 128bits UUID service description
struct attm_desc_128
{
    /// 128 bits UUID LSB First
    uint8_t uuid[ATT_UUID128_LEN];
    /// Attribute Permissions (@see enum attm_perm_mask)
    uint16_t perm;
    /// Attribute Extended Permissions (@see enum attm_value_perm_mask)
    uint16_t ext_perm;
    /// Attribute Max Size
    /// note: for characteristic declaration contains handle offset
    /// note: for included service, contains target service handle
    uint16_t max_size;
};


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Function use to ease service database creation.
 * Use @see attmdb_add_service function of attmdb module to create service database,
 * then use @see attmdb_add_attribute function of attmdb module to create attributes
 * according to database description array given in parameter.
 *
 * @note: database description array shall be const to reduce memory consumption (only ROM)
 * @note: It supports only 16 bits UUIDs
 *
 * @note: If shdl = 0, it return handle using first available handle (shdl is
 * modified); else it verifies if start handle given can be used to allocates handle range.
 *
 * @param[in|out] shdl          Service start handle.
 * @param[in]     uuid          Service UUID
 * @param[in|out] cfg_flag      Configuration Flag, each bit matches with an attribute of
 *                              att_db (Max: 32 attributes); if the bit is set to 1, the
 *                              attribute will be added in the service.
 * @param[in]     max_nb_att    Number of attributes in the service
 * @param[in|out] att_tbl       Array which will be fulfilled with the difference between
 *                              each characteristic handle and the service start handle.
 *                              This array is useful if several characteristics are optional
 *                              within the service, can be set to NULL if not needed.
 * @param[in]     dest_id       Task ID linked to the service. This task will be notified
 *                              each time the service content is modified by a peer device.
 * @param[in|out] att_db        Table containing all attributes information
 * @param[in]     svc_perm      Service permission (@see enum attm_svc_perm_mask)
 *
 * @return Command status code:
 *  - @ref LE_SUCCESS: If database creation succeeds.
 *  - @ref ATT_ERR_INVALID_HANDLE: If start_hdl given in parameter + nb of attribute override
 *                            some existing services handles.
 *  - @ref ATT_ERR_INSUFF_RESOURCE: There is not enough memory to allocate service buffer.
 *                           or of new attribute cannot be added because all expected
 *                           attributes already added or buffer overflow detected during
 *                           allocation
 ****************************************************************************************
 */
uint8_t attm_svc_create_db(uint16_t *shdl, uint16_t uuid, uint8_t *cfg_flag, uint8_t max_nb_att,
                           uint8_t *att_tbl, task_id_t dest_id,
                           const struct attm_desc *att_db, uint8_t svc_perm);

/**
 ****************************************************************************************
 * @brief Function use to ease service database creation.
 * Use @see attmdb_add_service function of attmdb module to create service database,
 * then use @see attmdb_add_attribute function of attmdb module to create attributes
 * according to database description array given in parameter.
 *
 * @note: database description array shall be const to reduce memory consumption (only ROM)
 * @note: It supports 128, 32 and 16 bits UUIDs
 *
 * @note: If shdl = 0, it return handle using first available handle (shdl is
 * modified); else it verifies if start handle given can be used to allocates handle range.
 *
 * @param[in|out] shdl          Service start handle.
 * @param[in]     uuid          Service UUID
 * @param[in|out] cfg_flag      Configuration Flag, each bit matches with an attribute of
 *                              att_db (Max: 32 attributes); if the bit is set to 1, the
 *                              attribute will be added in the service.
 * @param[in]     max_nb_att    Number of attributes in the service
 * @param[in|out] att_tbl       Array which will be fulfilled with the difference between
 *                              each characteristic handle and the service start handle.
 *                              This array is useful if several characteristics are optional
 *                              within the service, can be set to NULL if not needed.
 * @param[in]     dest_id       Task ID linked to the service. This task will be notified
 *                              each time the service content is modified by a peer device.
 * @param[in|out] att_db        Table containing all attributes information
 * @param[in]     svc_perm      Service permission (@see enum attm_svc_perm_mask)
 *
 * @return Command status code:
 *  - @ref LE_SUCCESS: If database creation succeeds.
 *  - @ref ATT_ERR_INVALID_HANDLE: If start_hdl given in parameter + nb of attribute override
 *                            some existing services handles.
 *  - @ref ATT_ERR_INSUFF_RESOURCE: There is not enough memory to allocate service buffer.
 *                           or of new attribute cannot be added because all expected
 *                           attributes already added or buffer overflow detected during
 *                           allocation
 ****************************************************************************************
 */
uint8_t attm_svc_create_db_128(uint16_t *shdl, const uint8_t* uuid, uint8_t *cfg_flag, uint8_t max_nb_att,
                               uint8_t *att_tbl, task_id_t dest_id,
                               const struct attm_desc_128 *att_db, uint8_t svc_perm);


/**
 ****************************************************************************************
 * @brief Function use to verify if several services can be allocated on a contiguous
 * handle range. If this command succeed, it means that service allocation will succeed.
 *
 * If start_hdl = 0, it return handle using first available handle (start_hdl is
 * modified); else it verifies if start handle given can be used to allocates handle range.
 *
 * @param[in|out] start_hdl     Service start handle.
 * @param[in]     nb_att        Number of handle to allocate (containing service handles)
 *
 * @return Command status code:
 *  - @ref LE_SUCCESS: If service allocation succeeds.
 *  - @ref ATT_ERR_INVALID_HANDLE: If start_hdl given in parameter or UUIDs value invalid
 ****************************************************************************************
 */
uint8_t attm_reserve_handle_range(uint16_t* start_hdl, uint8_t nb_att);

/**
 ****************************************************************************************
 * @brief Update attribute value
 *
 * Updating attribute value do not trigger any notification or indication, this shall be
 * handled by GATT task.
 *
 * @param[in] handle Attribute handle.
 * @param[in] length Size of new attribute value
 * @param[in] offset Data offset of in the payload to set
 * @param[in] value  Attribute value payload
 *
 * @return Command status code:
 *  - @ref LE_SUCCESS: If attribute value update succeeds
 *  - @ref ATT_ERR_INVALID_HANDLE: If handle doesn't exist in database
 *  - @ref ATT_ERR_REQUEST_NOT_SUPPORTED: If attribute data not present in database or
 *                                        cannot be modified
 *  - @ref ATT_ERR_INVALID_ATTRIBUTE_VAL_LEN: If new value length exceeds maximum attribute
 *                              value length.
 *
 ****************************************************************************************
 */
uint8_t attm_att_set_value(uint16_t handle, att_size_t length, att_size_t offset, uint8_t* value);

/**
 ****************************************************************************************
 * @brief Retrieve attribute value

 *
 * @param[in]  handle Attribute handle.
 * @param[out] length Size of attribute value
 * @param[out] value  Pointer to attribute value payload
 *
 * @return Command status code:
 *  - @ref LE_SUCCESS: If request succeeds
 *  - @ref ATT_ERR_INVALID_HANDLE: If handle doesn't exist in database
 *  - @ref ATT_ERR_REQUEST_NOT_SUPPORTED: If attribute data not present in database
 ****************************************************************************************
 */
uint8_t attm_get_value(uint16_t handle, att_size_t* length, uint8_t** value);

/**
 ****************************************************************************************
 * @brief Update attribute permission
 *
 * @param[in] handle Attribute handle.
 *  - @ref ATT_ERR_REQUEST_NOT_SUPPORTED: If attribute data not present in database
 * @param[in] perm       New attribute permission
 * @param[in] ext_perm   New attribute extended permission
 *
 * @return Command status code:
 *  - @ref LE_SUCCESS: If request succeeds
 *  - @ref ATT_ERR_INVALID_HANDLE: If handle doesn't exist in database
 *  - @ref ATT_ERR_REQUEST_NOT_SUPPORTED: If attribute permission is fixed
 ****************************************************************************************
 */
uint8_t attm_att_set_perm(uint16_t handle, uint16_t perm, uint16_t ext_perm);

/**
 ****************************************************************************************
 * @brief Reset some permissions bit in the Handle passed as parameter.
 *
 * @param[in] handle      Attribute handle.
 * @param[in] access_mask Access mask of permission to update
 * @param[in] perm        New value of the permission to update
 *
 *
 * @return Command status code:
 *  - @ref LE_SUCCESS: If request succeeds
 *  - @ref ATT_ERR_INVALID_HANDLE: If handle doesn't exist in database
 *  - @ref ATT_ERR_REQUEST_NOT_SUPPORTED: If attribute permission is fixed
 ****************************************************************************************
 */
uint8_t attm_att_update_perm(uint16_t handle, uint16_t access_mask, uint16_t perm);

/**
 ****************************************************************************************
 * @brief Update attribute service permission
 *
 * @param[in] handle Attribute handle.
 * @param[in] perm   New attribute permission
 *
 * @return Command status code:
 *  - @ref LE_SUCCESS: If request succeeds
 *  - @ref ATT_ERR_INVALID_HANDLE: If handle doesn't exist in database
 ****************************************************************************************
 */
uint8_t attm_svc_set_perm(uint16_t handle, uint8_t perm);

/**
 ****************************************************************************************
 * @brief Retrieve attribute service permission
 *
 * @param[in]  handle Attribute handle.
 * @param[out] perm   Permission value to return
 *
 * @return Command status code:
 *  - @ref LE_SUCCESS: If request succeeds
 *  - @ref ATT_ERR_INVALID_HANDLE: If handle doesn't exist in database
 ****************************************************************************************
 */
uint8_t attm_svc_get_perm(uint16_t handle, uint8_t* perm);

/**
 ****************************************************************************************
 * @brief Control visibility of a service from peer device. service present in database
 *        but cannot be access by a peer device.
 *
 * @param[in] handle Service handle.
 * @param[in] hide   True to hide the service, False to restore visibility
 *
 * @return Command status code:
 *  - @ref LE_SUCCESS: If service allocation succeeds.
 *  - @ref ATT_ERR_INVALID_HANDLE: If start_hdl given in parameter or UUIDs value invalid
 ****************************************************************************************
 */
uint8_t attmdb_svc_visibility_set(uint16_t handle, bool hide);

/**
 ****************************************************************************************
 * @brief Clear database
 *
 * For debug purpose only, this function clear the database and unalloc all services
 * within database.
 *
 * This function shall be used only for qualification and tests in order to manually
 * change database without modifying software.
 ****************************************************************************************
 */
void attmdb_destroy(void);

/**
 ****************************************************************************************
 * @brief Compare if two UUIDs matches
 *
 * @param[in]  uuid_a      UUID A value
 * @param[in]  uuid_a_len  UUID A length
 * @param[in]  uuid_b      UUID B value
 * @param[in]  uuid_b_len  UUID B length
 *
 * @return true if UUIDs matches, false otherwise
 ****************************************************************************************
 */
bool attm_uuid_comp(uint8_t *uuid_a, uint8_t uuid_a_len, uint8_t *uuid_b, uint8_t uuid_b_len);

/**
 ****************************************************************************************
 * @brief Check if two UUIDs matches (2nd UUID is a 16 bits UUID with LSB First)
 *
 * @param[in]  uuid_a      UUID A value
 * @param[in]  uuid_a_len  UUID A length
 * @param[in]  uuid_b      UUID B 16 bit value
 *
 * @return true if UUIDs matches, false otherwise
 ****************************************************************************************
 */
bool attm_uuid16_comp(uint8_t *uuid_a, uint8_t uuid_a_len, uint16_t uuid_b);

/**
 ****************************************************************************************
 * @brief Convert UUID value to 128 bit UUID
 *
 * @param[out] uuid128   converted 32-bit Bluetooth UUID to 128-bit UUID
 * @param[in]  uuid      UUID to convert to 128-bit UUID
 * @param[in]  uuid_len  UUID length
 *
 ****************************************************************************************
 */
void attm_convert_to128(uint8_t *uuid128, uint8_t *uuid, uint8_t uuid_len);

/**
 ****************************************************************************************
 * @brief Check if it's a Bluetooth 16-bits UUID for 128-bit input
 *
 * @param[in]  uuid      128-bit UUID
 *
 * @return true if uuid  is a Bluetooth 16-bit UUID, false else.
 ****************************************************************************************
 */
bool attm_is_bt16_uuid(uint8_t *uuid);

/**
 ****************************************************************************************
 * @brief Check if it's a Bluetooth 32 bits UUID for 128-bit input
 *
 * @param[in]  uuid      128-bit UUID
 *
 * @return true if uuid  is a Bluetooth 32-bits UUID, false else.
 ****************************************************************************************
 */
bool attm_is_bt32_uuid(uint8_t *uuid);


#endif // ATTM_API_H_
