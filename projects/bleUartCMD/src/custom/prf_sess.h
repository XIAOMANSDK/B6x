/**
 ****************************************************************************************
 *
 * @file prf_sess.h
 *
 * @brief Header file - Serial Service Profile (Server Role).
 *
 ****************************************************************************************
 */

#ifndef PRF_SESS_H_
#define PRF_SESS_H_

#include <stdint.h>

#include "att.h"
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
    #define SES_READ_SUP            (1)
#endif

/// Callback of Client Config enable
#if !defined(SES_CLI_CFG)
    #define SES_CLI_CFG             (0)
#endif


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */
extern uint8_t sess_inx_nmb;
extern uint8_t SES_ATT_IDX[];

extern uint8_t ses_perm_nmb;
extern uint16_t ses_atts_perm[];

typedef struct 
{
    uint8_t length;
    uint8_t data[31];    
}READ_INFO_T;

extern uint8_t sess_read_nmb;
extern READ_INFO_T ses_read_info[];


typedef struct 
{
    uint8_t uuid[16];    
}UUID_INFO_T;
extern uint8_t sess_uuid_nmb;
extern UUID_INFO_T sess_uuid[];

/// Server Environment Variable
typedef struct sess_env_tag
{
    // Service Start Handle
    uint16_t  start_hdl;
    // Client Config of peer devices - each 2Bits(NTF & IND), so max_peer=8.
    uint16_t  ntf_bits;
    // Number of notify pkt
    uint8_t   nb_pkt;
} sess_env_t;

extern sess_env_t sess_env[];

/// Attributes Index
enum ses_att_idx
{
    // Service Declaration, *MUST* Start at 0
    SES_IDX_SVC,
    
    // Serial TXD Char.
    SES_IDX_TXD_CHAR,
    SES_IDX_TXD_VAL,
    SES_IDX_TXD_NTF_CFG,

    // Serial RXD Char.
    SES_IDX_RXD_CHAR,
    SES_IDX_RXD_VAL, //5

    #if (SES_READ_SUP)
    // Serial READ Char.
    SES_IDX_READ_CHAR,
    SES_IDX_READ_VAL,
    #endif

    // Max Index, *NOTE* Minus 1(Svc Decl) is .nb_att
    SES_IDX_NB,
};

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

uint8_t sess_txd_send1(uint16_t handle, uint16_t len, const uint8_t *data);

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
