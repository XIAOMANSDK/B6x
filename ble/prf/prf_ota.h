/**
 ****************************************************************************************
 *
 * @file prf_ota.h
 *
 * @brief Header file - OTA Service Profile (Server Role).
 *
 ****************************************************************************************
 */

#ifndef PRF_OTA_H_
#define PRF_OTA_H_

#include <stdint.h>
#include <stdbool.h>


/*
 * DEFINES
 ****************************************************************************************
 */

/// Service Start Handle(0 = dynamically allocated)
#ifndef OTA_START_HDL
    #define OTA_START_HDL           (0)
#endif

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Add OTA Service Profile in the DB.
 *        Customize via pre-define @see OTA_START_HDL
 *
 * @return Result status, LE_SUCCESS or Error Reason
 ****************************************************************************************
 */
uint8_t ota_svc_init(void);

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
uint8_t ota_ntf_send(uint8_t conidx, uint16_t len, const uint8_t *data);

/**
 ****************************************************************************************
 * @brief Callback on received data from peer device via WC or WQ (__WEAK func)
 *
 * @param[in] conidx   peer device connection index
 * @param[in] len      Length of data
 * @param[in] data     pointer of buffer
 ****************************************************************************************
 */
void ota_recv(uint8_t conidx, uint16_t len, const uint8_t *data);
#endif /* PRF_OTA_H_ */
