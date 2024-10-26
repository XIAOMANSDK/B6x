/**
 ****************************************************************************************
 *
 * @file prf_scps.h
 *
 * @brief Header file - Scan Parameters Profile Server.
 *
 ****************************************************************************************
 */

#ifndef PRF_SCPS_H_
#define PRF_SCPS_H_

#include <stdint.h>


/*
 * DEFINES
 ****************************************************************************************
 */

/// Server Start Handle(0 = dynamically allocated)
#if !defined(SCP_START_HDL)
    #define SCP_START_HDL           (0)
#endif

/// Scan Refresh Characteristic(Support or Not)
#if !defined(SCP_FEATURES)
    #define SCP_FEATURES            (SCP_SCAN_REFRESH_SUP)
#endif

/// Scanning set parameters range min
#define SCP_SCAN_INTERVAL_MIN       0x04
/// Scanning set parameters range max
#define SCP_SCAN_INTERVAL_MAX       0x4000

/// Scanning set parameters range min
#define SCP_SCAN_WINDOW_MIN         0x04
/// Scanning set parameters range max
#define SCP_SCAN_WINDOW_MAX         0x4000

/// Features Flag Masks
enum scp_features
{
    /// Scan Refresh Characteristic is not supported
    SCP_SCAN_REFRESH_NOT_SUP = 0,
    /// Scan Refresh Characteristic is supported
    SCP_SCAN_REFRESH_SUP,
};


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Add Scan Parameters Profile in the DB
 *        Customize via pre-define @see SCP_START_HDL @see SCP_FEATURES
 *
 * @return Result status, LE_SUCCESS or Error Reason
 ****************************************************************************************
 */
uint8_t scps_svc_init(void);

/**
 ****************************************************************************************
 * @brief Enable SCP Notification Configurations
 *
 * @param[in] conidx     Connection index
 * @param[in] ntf_cfg    Scan Refresh Notification Config @see prf_cli_conf
 ****************************************************************************************
 */
void scps_set_ccc(uint8_t conidx, uint8_t ntf_cfg);

/**
 ****************************************************************************************
 * @brief Send a scan refresh to peer device via NTF
 *
 * @param[in] conidx     Connection index
 *
 * @return Status of the operation @see prf_err
 ****************************************************************************************
 */
uint8_t scps_scan_refresh(uint8_t conidx);

/**
 ****************************************************************************************
 * @brief Callback on received scan params from peer device via WC (__weak func)
 *
 * @param[in] conidx     Connection index
 * @param[in] scan_intv  Scan interval value
 * @param[in] scan_wd    Scan window value
 *
 ****************************************************************************************
 */
void scps_cb_scan_param(uint8_t conidx, uint16_t scan_intv, uint16_t scan_wd);


#endif /* PRF_SCPS_H_ */
