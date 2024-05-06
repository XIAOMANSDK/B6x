/**
 ****************************************************************************************
 *
 * @file prf_api.h
 *
 * @brief Header file - BLE Profiles' API functions, all inlcuded.
 *
 ****************************************************************************************
 */

#ifndef PRF_API_H_
#define PRF_API_H_

#include <stdint.h>


/*
 * DEFINES
 ****************************************************************************************
 */

/// Profile Enable
#if !defined(PRF_GATT)
    #define PRF_GATT            (1)
#endif

/// Service Start Handle(0 = dynamically allocated)
#if !defined(GAP_START_HDL)
    #define GAP_START_HDL       (0)
#endif

#if !defined(GATT_START_HDL)
    #define GATT_START_HDL      (0)
#endif

#if !defined(MESH_START_HDL)
    #define MESH_START_HDL      (0)
#endif

#if !defined(MESH_SVC_CFG)
    #define MESH_SVC_CFG        (0x03) //(MESH_ATT_PROV | MESH_ATT_PROXY)
#endif

/// Attributes present in GAP Service
#if !defined(GAP_ATT_CFG)
#if (PRF_HIDS)
    #define GAP_ATT_CFG         (0x40) //(GAP_ATT_SLV_PREF_PAR_BIT)
#else
    #define GAP_ATT_CFG         (0)
#endif //(PRF_HIDS)
#endif

/// GAP Attribute write permission requirement
enum gap_att_write_perm
{
    /// Disable write access
    GAP_ATT_WRITE_DISABLE       = 0,
    /// Enable write access - no authentication required
    GAP_ATT_WRITE_NO_AUTH       = 1,
    /// Write access requires unauthenticated link
    GAP_ATT_WRITE_UNAUTH        = 2,
    /// Write access requires authenticated link
    GAP_ATT_WRITE_AUTH          = 3,
    /// Write access requires secure connected link
    GAP_ATT_WRITE_SEC_CON       = 4
};

/// GAP Attribute Configuration
///   15   14   13   12   11   10    9    8    7    6    5    4    3    2    1    0
/// +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
/// |                    RFU                     |PCP |   ICON_PERM  |   NAME_PERM  |
/// +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
/// - Bit [0-2] : Device Name write permission requirements for peer device (@see gap_att_write_perm)
/// - Bit [3-5] : Device Appearance write permission requirements for peer device (@see gap_att_write_perm)
/// - Bit [6]   : Slave Preferred Connection Parameters present
/// - Bit [7-15]: Reserved
enum gap_att_cfg
{
    /// Device Name write permission requirements for peer device
    GAP_ATT_NAME_PERM_MASK      = 0x0007,
    GAP_ATT_NAME_PERM_LSB       = 0,
    /// Device Appearance write permission requirements for peer device
    GAP_ATT_ICON_PERM_MASK      = 0x0038,
    GAP_ATT_ICON_PERM_LSB       = 3,
    /// Slave Preferred Connection Parameters present in GAP Service.
    GAP_ATT_SLV_PREF_PAR_BIT    = 0x0040,
    GAP_ATT_SLV_PREF_PAR_POS    = 6,
};

/// Mesh Attribute Configuration
enum mesh_att_cfg
{
    /// Provisioning attribute
    MESH_ATT_PROV               = (1 << 0),

    /// Proxy attribute
    MESH_ATT_PROXY              = (1 << 1),
};


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Add Generic Access Profile(0x1800) in the DB.
 *
 * @param[in] start_hdl  Service start handle
 * @param[in] att_cfg    Attribute configuration @see gap_att_cfg
 *
 * @return Result status, LE_SUCCESS or Error Reason
 ****************************************************************************************
 */
uint8_t gap_svc_init(uint16_t start_hdl, uint16_t att_cfg);

/**
 ****************************************************************************************
 * @brief Retrieve Dev Info to Generic Access Profile, User implement for Callback.
 *
 * @param[in]  conidx  connection index
 * @param[in]  req     request of info type @see enum gapc_dev_info
 * @param[in]  maxlen  buffer length, DEV_NAME_MAX_LEN or size of gapc_conn_param
 * @param[out] info    pointer of buffer
 *
 * @return Length of device information, 0 means an error occurs.
 ****************************************************************************************
 */
uint16_t gap_svc_get_dev_info(uint8_t conidx, uint8_t req, uint16_t maxlen, uint8_t *info);

/**
 ****************************************************************************************
 * @brief Peer update device name or appearance to Generic Access Profile, default reject.
 *
 * @param[in]  conidx  connection index
 * @param[in]  req     GAPC_DEV_NAME or GAPC_DEV_APPEARANCE @see enum gapc_dev_info
 * @param[in]  len     length of info data
 * @param[out] info    pointer of data
 *
 * @return status of accept(GAP_ERR_NO_ERROR) or reject(GAP_ERR_REJECTED).
 ****************************************************************************************
 */
uint8_t gap_svc_set_dev_info(uint8_t conidx, uint8_t req, uint16_t len, const uint8_t *info);

/**
 ****************************************************************************************
 * @brief Add Generic Attribute Profile(0x1801) in the DB.
 *
 * @param[in] start_hdl  Service start handle
 *
 * @return Result status, LE_SUCCESS or Error Reason
 ****************************************************************************************
 */
uint8_t gatt_svc_init(uint16_t start_hdl);

/**
 ****************************************************************************************
 * @brief Add Mesh Profiles in the DB.
 *
 * @param[in] start_hdl  Service start handle
 * @param[in] svc_cfg    Service configuration
 *
 * @return Result status, LE_SUCCESS or Error Reason
 ****************************************************************************************
 */
uint8_t mesh_svc_init(uint16_t start_hdl, uint16_t svc_cfg);


/*
 * STANDARD PROFILES
 ****************************************************************************************
 */

/// Device Information Service Server
#include "prf_diss.h"

/// Battery Service Server
#include "prf_bass.h"

/// HID Over GATT Profile Server
#include "prf_hids.h"

#if (PRF_SCPS)
/// Scan Parameters Profile Server
#include "prf_scps.h"
#endif

/// user add more...


/*
 * CUSTOMIZED PROFILES
 ****************************************************************************************
 */

/// Serial Service Profile
#include "prf_sess.h"

#if (PRF_OTAS)
/// OTA Service Profile
#include "prf_otas.h"
#endif

#if (PRF_PTSS)
/// Profile Testing Service
#include "prf_ptss.h"
#endif

/// user add more...


#endif /* PRF_API_H_ */
