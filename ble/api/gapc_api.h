/**
 ****************************************************************************************
 *
 * @file gapc_api.h
 *
 * @brief Generic Access Profile Controller API functions.
 *
 ****************************************************************************************
 */

#ifndef GAPC_API_H_
#define GAPC_API_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include <stdint.h>
#include "gapc.h"


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Response GAPC_CONNECTION_REQ_IND with specific link data configuration.
 *
 * @param[in] conidx   Connection index
 * @param[in] auth     Authentication(@see gap_auth)
 ****************************************************************************************
 */
void gapc_connect_rsp(uint8_t conidx, uint8_t auth);

/**
 ****************************************************************************************
 * @brief Response GAPC_PARAM_UPDATE_REQ_IND(Master confirm or not).
 *
 * @param[in] conidx   Connection index
 * @param[in] accept   True or False to accept slave connection parameters
 * @param[in] ce_min   Minimum Connection Event Duration
 * @param[in] ce_max   Maximum Connection Event Duration
 ****************************************************************************************
 */
void gapc_param_update_rsp(uint8_t conidx, bool accept, uint16_t ce_min, uint16_t ce_max);

/**
 ****************************************************************************************
 * @brief Set the PHY configuration for current active link.
 *
 * @param[in] conidx   Connection index
 * @param[in] tx_phy   PHY for data transmission(@see enum gap_phy)
 * @param[in] rx_phy   PHY for data reception(@see enum gap_phy)
 * @param[in] phy_opt  PHY options(@see enum gapc_phy_option)
 *
 *@return Message GAPC_LE_PHY_IND
 ****************************************************************************************
 */
void gapc_update_phy(uint8_t conidx, uint8_t tx_phy, uint8_t rx_phy, uint8_t phy_opt);

/**
 ****************************************************************************************
 * @brief Set the BLE Data Length Extension for current active link.
 *
 * @param[in] conidx     Connection index
 * @param[in] tx_octets  Length of data transmission(@see LE_MAX_OCTETS)
 * @param[in] tx_time    Time of data transmission(@see LE_MAX_TIME)
 *
 *@return Message GAPC_LE_PKT_SIZE_IND
 ****************************************************************************************
 */
void gapc_update_dle(uint8_t conidx, uint16_t tx_octets, uint16_t tx_time);

/**
 ****************************************************************************************
 * @brief Perform update of connection parameters command.
 *
 * @param[in] conidx   Connection index
 * @param[in] param    Connection parameters
 *
 *@return Message GAPC_PARAM_UPDATED_IND
 ****************************************************************************************
 */
void gapc_update_param(uint8_t conidx, struct gapc_conn_param const* param);

/**
 ****************************************************************************************
 * @brief Request disconnection of current link command.
 *
 * @param[in] conidx   Connection index
 * @param[in] reason   Reason of disconnection
 *
 *@return Message GAPC_DISCONNECT_IND
 ****************************************************************************************
 */
void gapc_disconnect(uint8_t conidx);

/**
 ****************************************************************************************
 * @brief Send gapc_get_info_cmd, Retrieve Connection information
 *
 * @param[in] conidx  Connection index
 * @param[in] info    Connection information (@see gapc_operation)
 ****************************************************************************************
 */
void gapc_get_info(uint8_t conidx, uint8_t info);

/**
 ****************************************************************************************
 * @brief Retrieve connection index from connection handle.
 *
 * @param[in] conhdl Connection handle
 *
 * @return Return found connection index, GAP_INVALID_CONIDX if not found.
 ****************************************************************************************
 */
uint8_t gapc_get_conidx(uint16_t conhdl);

/**
 ****************************************************************************************
 * @brief Retrieve connection handle from connection index.
 *
 * @param[in] conidx Connection index
 *
 * @return Return found connection handle, GAP_INVALID_CONHDL if not found.
 ****************************************************************************************
 */
uint16_t gapc_get_conhdl(uint8_t conidx);

/**
 ****************************************************************************************
 * @brief Retrieve connection role from connection index.
 *
 * @param[in] conidx Connection index
 *
 * @return Return found connection role
 ****************************************************************************************
 */
uint8_t gapc_get_role(uint8_t conidx);

/**
 ****************************************************************************************
 * @brief Set resolvable address used for connection establishment as local address.
 *
 * @brief conidx    Connection index
 * @brief p_addr    Pointer to the resolvable address used by either advertising activity or
 * resolvable activity.
 ****************************************************************************************
 */
void gapc_set_local_addr(uint8_t conidx, uint8_t *p_addr);

/**
 ****************************************************************************************
 * @brief Retrieve connection address information on current link.
 *
 * @param[in] conidx Connection index
 * @param[in] src    information source(@see gapc_smp_addr_src)
 *
 * @return Return found connection address
 ****************************************************************************************
 */
struct gap_bdaddr* gapc_get_bdaddr(uint8_t conidx, uint8_t src);

/**
 ****************************************************************************************
 * @brief Check if current link support security requirements.
 *
 * @param[in] conidx  Connection index
 * @param[in] sec_req Link security requirement to test
 *
 * @return True if link requirement is supported, False else.
 ****************************************************************************************
 */
bool gapc_is_sec_set(uint8_t conidx, uint8_t sec_req);

/**
 ****************************************************************************************
 * @brief Retrieve Link Security level
 *
 * @param[in] conidx  Connection index
 *
 * @return Link Security level.
 ****************************************************************************************
 */
uint8_t gapc_lk_sec_lvl_get(uint8_t conidx);

/**
 ****************************************************************************************
 * @brief Retrieve the encryption key size of the connection
 *
 * @param[in] conidx Connection index
 *
 * @return encryption key size (size is 7 - 16 byte range)
 *
 ****************************************************************************************
 */
uint8_t gapc_enc_keysize_get(uint8_t conidx);

/**
 ****************************************************************************************
 * @brief Send bond request(Master Role).
 *
 * @param[in] conidx   Connection index
 * @param[in] feat     Pairing Feature(@see gapc_pairing)
 ****************************************************************************************
 */
void gapc_smp_bond_req(uint8_t conidx, struct gapc_pairing const* feat);

/**
 ****************************************************************************************
 * @brief Send encrypt request(Master Role).
 *
 * @param[in] conidx   Connection index
 * @param[in] ltk      Long term key(@see gapc_ltk)
 ****************************************************************************************
 */
void gapc_smp_encrypt_req(uint8_t conidx, struct gapc_ltk const* ltk);

/**
 ****************************************************************************************
 * @brief Send security request.
 *
 * @param[in] conidx   Connection index
 * @param[in] auth     Authentication req(@see gap_auth)
 ****************************************************************************************
 */
void gapc_smp_security_req(uint8_t conidx, uint8_t auth);

/**
 ****************************************************************************************
 * @brief Send Keypress Notification.
 *
 * @param[in] conidx   Connection index
 * @param[in] keyntf   Keypress type req(@see gapc_key_ntf_type)
 ****************************************************************************************
 */
void gapc_key_press_notify(uint8_t conidx, uint8_t keyntf);

/**
 ****************************************************************************************
 * @brief Respond to peer pairing information request(GAPC_PAIRING_REQ)
 *
 * @param[in] conidx  Connection Index
 * @param[in] feat    Pairing feature information, NULL means pairing rejected
 *
 * @return status of pairing
 ****************************************************************************************
 */
uint8_t gapc_smp_pairing_rsp(uint8_t conidx, struct gapc_pairing *feat);

/**
 ****************************************************************************************
 * @brief Handles LTK exchange part of pairing(GAPC_BOND_REQ_IND)
 *
 * @param[in] conidx  Connection Index
 * @param[in] ltk     The Long Term Key transmitted by application
 *
 * @return status of pairing
 ****************************************************************************************
 */
uint8_t gapc_smp_pairing_ltk_exch(uint8_t conidx, struct gapc_ltk* ltk);

/**
 ****************************************************************************************
 * @brief Handles CSRK exchange part of pairing(GAPC_BOND_REQ_IND)
 *
 * @param[in] conidx   Connection Index
 * @param[in] csrk     The Connection signature resolving key transmitted by application
 *
 * @return status of pairing
 ****************************************************************************************
 */
uint8_t gapc_smp_pairing_csrk_exch(uint8_t conidx, struct gap_sec_key *csrk);

/**
 ****************************************************************************************
 * @brief Handles TK exchange part of pairing(GAPC_BOND_REQ_IND)
 *
 * @param[in] conidx  Connection Index
 * @param[in] accept  True if pairing is accepted, False else
 * @param[in] tk      The Temporary Key transmitted by application
 *
 * @return status of pairing
 ****************************************************************************************
 */
uint8_t gapc_smp_pairing_tk_exch(uint8_t conidx, bool accept,  struct gap_sec_key *tk);

#if (SEC_CON_ENB)
/**
 ****************************************************************************************
 * @brief Handles OOB exchange part of pairing(GAPC_BOND_REQ_IND)
 *
 * @param[in] conidx  Connection Index
 * @param[in] accept  Accept or Reject the OOB (reject if OOB reception not available on the device)
 * @param[in] csrk    The OOB Confirm and OOB Rand from the peer
 *
 * @return status of pairing
 ****************************************************************************************
 */
uint8_t gapc_smp_pairing_oob_exch(uint8_t conidx, bool accept, struct gapc_oob *oob);

/**
 ****************************************************************************************
 * @brief Handles Numeric Value Acceptance as part of pairing(GAPC_BOND_REQ_IND)
 *
 * @param[in] conidx  Connection Index
 * @param[in] accept  Accept or Reject the numeric comparison
 *
 * @return status of pairing
 ****************************************************************************************
 */
uint8_t gapc_smp_pairing_nc_exch(uint8_t conidx, uint8_t accept);
#endif //(SEC_CON_ENB)

/**
 ****************************************************************************************
 * @brief Slave respond to peer device encryption request(GAPC_ENCRYPT_REQ_IND)
 *
 * @param[in] conidx   Connection Index
 * @param[in] key_size Encryption key size(7~16)
 * @param[in] ltk      16-bytes LTK(Accept) or NULL(Reject) to start encryption
 ****************************************************************************************
 */
uint8_t gapc_smp_encrypt_cfm(uint8_t conidx, uint8_t key_size, struct gap_sec_key *ltk);

#endif // GAPC_API_H_
