/**
 ****************************************************************************************
 *
 * @file gapm_api.h
 *
 * @brief Generic Access Profile Manager API functions.
 *
 ****************************************************************************************
 */

#ifndef GAPM_API_H_
#define GAPM_API_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include <stdint.h>
#include <stdbool.h>
#include "gapm.h"


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Reset link layer and the host
 *
 * @return Message GAPM_CMP_EVT(operation GAPM_RESET)
 ****************************************************************************************
 */
void gapm_reset(void);

/**
 ****************************************************************************************
 * @brief Initial device configuration
 *
 * @param[in] cfg Configure
 *
 * @return Message GAPM_CMP_EVT(operation GAPM_SET_DEV_CONFIG)
 ****************************************************************************************
 */
void gapm_set_dev(const struct gapm_dev_config *cfg, const bd_addr_t *bdaddr, const uint8_t *irk);

/**
 ****************************************************************************************
 * @brief Generate a 8-byte random number
 *
 * @return Message GAPM_CMP_EVT(operation GAPM_GEN_RAND_NB)
 ****************************************************************************************
 */
void gapm_gen_rand(void);

/**
 ****************************************************************************************
 * @brief Modify current IRK
 *
 * @return Message GAPM_CMP_EVT(operation GAPM_SET_IRK)
 ****************************************************************************************
 */
void gapm_set_irk(uint8_t *irk);

/**
 ****************************************************************************************
 * @brief Create activity.
 *
 * @param[in] actv_type  Activity type(@see gapm_actv_type)
 * @param[in] addr_type  Own addr type(@see gapm_own_addr)
 *
 * @return Message GAPM_CMP_EVT(operation GAPM_CREATE_XXX_ACTIVITY) and GAPM_ACTIVITY_CREATED_IND
 ****************************************************************************************
 */
void gapm_create_activity(uint8_t actv_type, uint8_t addr_type);

/**
 ****************************************************************************************
 * @brief Start activity.
 *
 * @param[in] actv_idx   Activity index
 * @param[in] addr_type  Own addr type(@see gapm_own_addr)
 *
 * @return Message GAPM_CMP_EVT(operation GAPM_START_ACTIVITY)
 ****************************************************************************************
 */
void gapm_start_activity(uint8_t actv_idx, uint16_t param_len, const void* param);

/**
 ****************************************************************************************
 * @brief Stop activity.
 *
 * @param[in] actv_idx  Activity index
 *
 * @return Message GAPM_CMP_EVT(operation GAPM_STOP_ACTIVITY) and GAPM_ACTIVITY_STOPPED_IND
 ****************************************************************************************
 */
void gapm_stop_activity(uint8_t actv_idx);

/**
 ****************************************************************************************
 * @brief Delete activity.
 *
 * @param[in] actv_idx Activity index
 *
 * @return Message GAPM_CMP_EVT(operation GAPM_DELETE_ACTIVITY)
 ****************************************************************************************
 */
void gapm_delete_activity(uint8_t actv_idx);

/**
 ****************************************************************************************
 * @brief Create advertising activity.
 *
 * @param[in] addr_type  Own addr type(@see gapm_own_addr)
 * @param[in] adv_param  Advertising parameters
 *
 * @return Message GAPM_CMP_EVT(operation GAPM_CREATE_ADV_ACTIVITY) and GAPM_ACTIVITY_CREATED_IND
 ****************************************************************************************
 */
void gapm_create_advertising(uint8_t addr_type, const struct gapm_adv_create_param *adv_param);

/**
 ****************************************************************************************
 * @brief Start advertising activity.
 *
 * @param[in] adv_idx   Activity index
 * @param[in] duration  Advertising duration, 0 for always on
 *
 * @return Message GAPM_CMP_EVT(operation GAPM_START_ACTIVITY)
 ****************************************************************************************
 */
void gapm_start_advertising(uint8_t adv_idx, uint16_t duration);

/// Macro of Stop advertising
#define gapm_stop_advertising(adv_idx) gapm_stop_activity(adv_idx)

/// Macro of Delete advertising
#define gapm_delete_advertising(adv_idx) gapm_delete_activity(adv_idx)

/**
 ****************************************************************************************
 * @brief Fill advertising data.
 *
 * @param[in] adv_idx  Activity index
 * @param[in] op_typ   Data operation(GAPM_SET_ADV_DATA or GAPM_SET_SCAN_RSP_DATA)
 *
 * @return Message GAPM_CMP_EVT(operation GAPM_SET_ADV_DATA or GAPM_SET_SCAN_RSP_DATA)
 ****************************************************************************************
 */
void gapm_set_adv_data(uint8_t adv_idx, uint8_t op_typ, uint16_t len, const uint8_t *data);

/**
 ****************************************************************************************
 * @brief Retrieve local public address.
 *
 * @return Return local public address
 ****************************************************************************************
 */
bd_addr_t* gapm_get_bdaddr(void);

/**
 ****************************************************************************************
 * @brief Retrieve the device address type (@see enum gapm_addr_type)
 *
 * @return address type
 ****************************************************************************************
 */
uint8_t gapm_get_address_type(void);

/**
 ****************************************************************************************
 * @brief Returns the local Public Key
 *
 * @return pointer to the local Public Key
 ****************************************************************************************
 */
public_key_t* gapm_get_local_public_key(void);

/**
 ****************************************************************************************
 * @brief Retrieve if Legacy pairing is supported on local device
 *
 * @return True if legacy pairing is supported
 ****************************************************************************************
 */
bool gapm_is_legacy_pairing_supp(void);

/**
 ****************************************************************************************
 * @brief Retrieve if Secure Connection pairing is supported on local device
 *
 * @return True if Secure Connection pairing is supported
 ****************************************************************************************
 */
bool gapm_is_sec_con_pairing_supp(void);

/**
 ****************************************************************************************
 * @brief Set the pairing mode is supported on local device
 *
 * @param[in] mode  Pairing supported (@see gapm_pairing_mode)
 ****************************************************************************************
 */
void gapm_set_pairing_supp(uint8_t mode);

/**
 ****************************************************************************************
 * @brief Return the maximal MTU value
 *
 * @return Maximal MTU value
 ****************************************************************************************
 */
uint16_t gapm_get_max_mtu(void);

/**
 ****************************************************************************************
 * @brief Set the maximal MTU value
 *
 * @param[in] mtu   Max MTU value (Minimum is 23)
 ****************************************************************************************
 */
void gapm_set_max_mtu(uint16_t mtu);

/**
 ****************************************************************************************
 * @brief Return the maximal MPS value
 *
 * @return Maximal MPS value
 ****************************************************************************************
 */
uint16_t gapm_get_max_mps(void);

/**
 ****************************************************************************************
 * @brief Set the maximal MPS value
 *
 * @param[in] mps   Max MPS value (Minimum is 23)
 ****************************************************************************************
 */
void gapm_set_max_mps(uint16_t mps);

/**
 ****************************************************************************************
 * @brief Get if preferred connection parameters present in GAP ATT database
 *
 * @return True if referred connection parameters present in GAP ATT database, False else
 ****************************************************************************************
 */
bool gapm_is_pref_con_param_pres(void);

/**
 ****************************************************************************************
 * @brief retrieve gap attribute handle from attribute index.
 *
 * @param[in] att_idx Attribute index
 *
 * @return Attribute handle
 ****************************************************************************************
 */
uint16_t gapm_get_att_handle(uint8_t att_idx);

/**
 ****************************************************************************************
 * @brief Check If Service changed feature is enabled or not
 *
 * @return true if enabled, false else.
 ****************************************************************************************
 */
bool gapm_svc_chg_en(void);


#endif // GAPM_API_H_
