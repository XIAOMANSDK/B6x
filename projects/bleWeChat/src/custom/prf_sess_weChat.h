/**
 ****************************************************************************************
 *
 * @file prf_sess.h
 *
 * @brief Header file - Serial Service Profile (Server Role).
 *
 ****************************************************************************************
 */

#ifndef PRF_SESS_AIRSYNC_H_
#define PRF_SESS_AIRSYNC_H_

#include <stdint.h>


/*
 * DEFINES
 ****************************************************************************************
 */

/// Service Start Handle(0 = dynamically allocated)
#if !defined(SES_START_HDL)
    #define SES_START_HDL           (0)
#endif

/// Use UUID 128bit or 16bit
#if !defined(SES_UUID_128)
    #define SES_UUID_128            (0)
#endif

/// Support READ Characteristic
#if !defined(SES_READ_SUP)
    #define SES_READ_SUP            (0)
#endif

/// Callback of Client Config enable
#if !defined(SES_CLI_CFG)
    #define SES_CLI_CFG             (0)
#endif


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Add Serial Service Profile in the DB.
 *        Customize via pre-define @see SES_START_HDL
 *
 * @return Result status, LE_SUCCESS or Error Reason
 ****************************************************************************************
 */
uint8_t sess_svc_init(void);

/**
 ****************************************************************************************
 * @brief Enable setting client configuration characteristics
 *
 * @param[in] conidx   Connection index
 * @param[in] cli_cfg  Client configuration @see prf_cli_conf
 ****************************************************************************************
 */
void sess_set_ccc(uint8_t conidx, uint8_t cli_cfg);

/**
 ****************************************************************************************
 * @brief Transmit data to peer device via NTF or IND
 *
 * @param[in] conidx   peer device connection index
 * @param[in] len      Length of data
 * @param[in] data     pointer of buffer
 *
 * @return Status of the operation @see prf_err
 ****************************************************************************************
 */
uint8_t sess_txd_send(uint8_t conidx, uint16_t len, const uint8_t *data);

uint8_t sess_ind_send(uint8_t conidx, uint16_t len, const uint8_t* data);
/**
 ****************************************************************************************
 * @brief Callback on received data from peer device via WC or WQ (__weak func)
 *
 * @param[in] conidx   peer device connection index
 * @param[in] len      Length of data
 * @param[in] data     pointer of buffer
 ****************************************************************************************
 */
void sess_cb_rxd(uint8_t conidx, uint16_t len, const uint8_t *data);

#if (SES_READ_SUP)
/**
 ****************************************************************************************
 * @brief Callback to response 'READ' from peer device (__weak func)
 *
 * @param[in] conidx  peer device connection index
 * @param[in] attidx  SESS attribute index, converted with 'handle'
 * @param[in] handle  SESS attribute handle to send read cfm
 *
 * @return Length of value been READ
 ****************************************************************************************
 */
void sess_cb_rdv(uint8_t conidx, uint8_t attidx, uint16_t handle);
#endif

#if (SES_CLI_CFG)
/**
 ****************************************************************************************
 * @brief Callback on enabled client config from peer device via WQ (__weak func)
 *
 * @param[in] conidx   Connection index
 * @param[in] cli_cfg  Client configuration @see prf_cli_conf
 ****************************************************************************************
 */
void sess_cb_ccc(uint8_t conidx, uint8_t cli_cfg);
#endif

#endif /* PRF_SESS_H_ */
