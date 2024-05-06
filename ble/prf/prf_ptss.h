/**
 ****************************************************************************************
 *
 * @file prf_ptss.h
 *
 * @brief Header file - Profile Testing Service (Server Role).
 *
 ****************************************************************************************
 */

#ifndef PRF_PTSS_H_
#define PRF_PTSS_H_

#include <stdint.h>


/*
 * DEFINES
 ****************************************************************************************
 */

/// Service Start Handle(0 = dynamically allocated)
#if !defined(PTS_START_HDL)
    #define PTS_START_HDL           (0x50)
#endif

/// Max length of received once
#define PTS_DATA_MAX_LEN            (22)
/// Max length of user description
#define PTS_DESC_MAX_LEN            (22)


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Add Service Profile in the DB
 *        Customize via pre-define @see PTS_START_HDL
 *
 * @return Result status, LE_SUCCESS or Error Reason
 ****************************************************************************************
 */
uint8_t ptss_svc_init(void);

/**
 ****************************************************************************************
 * @brief Enable setting client configuration characteristics
 *
 * @param[in] conidx   Connection index
 * @param[in] cli_cfg  Client configuration @see prf_cli_conf
 ****************************************************************************************
 */
void ptss_set_ccc(uint8_t conidx, uint8_t cli_cfg);

/**
 ****************************************************************************************
 * @brief Transmit data to peer device via NTF or IND
 *
 * @param[in] conidx   peer destination connection index
 * @param[in] handle   Handle of NTF/IND
 * @param[in] len      Length of data
 * @param[in] data     pointer of buffer
 *
 * @return Status of the operation @see prf_err
 ****************************************************************************************
 */
uint8_t ptss_evt_send(uint8_t conidx, uint16_t handle, uint16_t len, const uint8_t* data);

/**
 ****************************************************************************************
 * @brief Callback on received data from peer device via WC or WQ (__weak func)
 *
 * @param[in] conidx   peer device connection index
 * @param[in] len      Length of data
 * @param[in] data     pointer of buffer
 ****************************************************************************************
 */
void ptss_cb_recv(uint8_t conidx, uint16_t len, const uint8_t *data);

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
void ptss_cb_read(uint8_t conidx, uint8_t attidx, uint16_t handle);

/**
 ****************************************************************************************
 * @brief Callback on enabled client config from peer device via WQ (__weak func)
 *
 * @param[in] conidx   Connection index
 * @param[in] cli_cfg  Client configuration @see prf_cli_conf
 ****************************************************************************************
 */
void ptss_cb_ccc(uint8_t conidx, uint8_t cli_cfg);

#endif /* PRF_PTSS_H_ */
