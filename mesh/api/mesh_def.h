/**
 ****************************************************************************************
 *
 * @file mesh_def.h
 *
 * @brief Header file for Mesh Stack Defines
 *
 ****************************************************************************************
 */

#ifndef _MESH_DEF_H_
#define _MESH_DEF_H_

/**
 ****************************************************************************************
 * @defgroup MESH_DEFINES Mesh Defines
 * @ingroup MESH
 * @brief  Mesh Defines
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDES FILES
 ****************************************************************************************
 */

#include <stdint.h>


/*
 * DEFINES
 ****************************************************************************************
 */

/// Unassigned address value
#define MESH_UNASSIGNED_ADDR            (0x0000)

/// Fixed group address - All proxies
#define MESH_PROXIES_ADDR               (0xFFFC)
/// Fixed group address - All friends
#define MESH_FRIENDS_ADDR               (0xFFFD)
/// Fixed group address - All relays
#define MESH_RELAYS_ADDR                (0xFFFE)
/// Fixed group address - All nodes
#define MESH_NODES_ADDR                 (0xFFFF)

/// Size of a key
#define MESH_KEY_LEN                    (16)
/// Size of value block when encrypting
#define MESH_ENC_VAL_LEN                (16)
/// Public Key X coordinate length
#define MESH_PUB_KEY_X_LEN              (32)
/// Public Key Y coordinate length
#define MESH_PUB_KEY_Y_LEN              (32)
/// Size of Device UUID
#define MESH_DEV_UUID_LEN               (16)
/// Length of the Private P-256 key
#define MESH_PRIVATE_KEY_LEN            (32)
/// ECDH Secret size length
#define MESH_ECDH_SECRET_LEN            (32)
/// Size of K1 result length value
#define MESH_K1_RES_LEN                 (16)
/// Size of K2 result length value - (263 bits)
#define MESH_K2_RES_LEN                 (33)
/// Size of K3 result length value - (64 bits)
#define MESH_K3_RES_LEN                 (8)
/// Size of K4 result length value - (6 bits)
#define MESH_K4_RES_LEN                 (1)
/// Size of the Nonce used for AES-CCM
#define MESH_NONCE_LEN                  (13)

/// Network identifier length
#define MESH_NET_ID_LEN                 (8)
/// Packet sequence number length
#define MESH_SEQ_LEN                    (3)
/// Address length
#define MESH_ADDR_LEN                   (2)
/// Key IDs length
#define MESH_KEY_ID_LEN                 (2)
/// Size of Label UUID
#define MESH_LABEL_UUID_LEN             (16)
/// Length of Network MIC for an Access Message
#define MESH_NMIC_ACC_LEN               (4)
/// Length of Network MIC for a Control Message
#define MESH_NMIC_CTL_LEN               (8)
/// Authentication data OOB length
#define MESH_OOB_AUTH_DATA_LEN          (16)
/// 64 bit random value
#define MESH_PROXY_RANDOM_LEN           (8)
/// 64 bit HASH value
#define MESH_PROXY_HASH_LEN             (8)

/// Perform Connectable Advertising during 60s
#define MESH_PROXY_CON_ADV_DUR          (60000)

/// Maximum Publish TTL value
#define MESH_TTL_MAX                    (0x7F)
/// Publish TTL value indicating that Default TTL  value must be used
#define MESH_TTL_USE_DEFAULT            (0xFF)

/// Maximum number of subnets the node can belong to
#define MESH_SUBNET_NB_MAX              (5)
/// Maximum number of models that can be registered on the node
#define MESH_MODEL_NB_MAX               (8)

/// Invalid Local identifier - m_lid_t
#define MESH_INVALID_LID                (0xFF)


/*
 * MACROS
 ****************************************************************************************
 */

/// Check if access opcode is a 1 octet value
#define MESH_IS_1_OCT_OPCODE(opcode)    (((opcode) & 0x80) == 0)
/// Check if access opcode is a 2 octets value
#define MESH_IS_2_OCT_OPCODE(opcode)    (((opcode) & 0xC0) == 0x80)
/// Check if access opcode is a 3 octets value
#define MESH_IS_3_OCT_OPCODE(opcode)    (((opcode) & 0xC0) == 0xC0)

/// Macro returning if a Model ID is a vendor Model ID
#define MESH_IS_VENDOR_MODEL(model_id)  ((model_id & 0xFFFF0000) != 0)


/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Connection packet type
enum mesh_bearer_con_type
{
    /// Network Message
    MS_BEARER_CON_TYPE_NET              = (0x00),
    /// Mesh Beacon message
    MS_BEARER_CON_TYPE_BEACON           = (0x01),
    /// Proxy configuration message
    MS_BEARER_CON_TYPE_PROXY            = (0x02),
    /// Provisioning PDU message
    MS_BEARER_CON_TYPE_PROV             = (0x03),
};

/// Proxy connectable advertising control values
enum mesh_proxy_adv_ctl
{
    /// Stop connectable advertising
    MS_PROXY_ADV_CTL_STOP               = 0,
    /// Start connectable advertising with Node Identity (duration = 60s)
    MS_PROXY_ADV_CTL_START_NODE,
    /// Start connectable advertising with Network ID (duration = 60s)
    MS_PROXY_ADV_CTL_START_NET,
};

/// Proxy connectable advertising state update types
enum mesh_proxy_adv_upd
{
    /// Advertising with Node Identity stopped
    MS_PROXY_ADV_NODE_STOP              = 0,
    /// Advertising with Node Identity started
    MS_PROXY_ADV_NODE_START,
    /// Advertising with Network ID stopped
    MS_PROXY_ADV_NET_STOP,
    /// Advertising with Node Identity started
    MS_PROXY_ADV_NET_START,
};

/// Proxy connectable advertising state update reasons
enum mesh_proxy_adv_upd_reason
{
    /// Stopped due to timeout (60s)
    MS_PROXY_ADV_UPD_REASON_TIMEOUT     = 0,
    /// Stopped due to state update
    MS_PROXY_ADV_UPD_REASON_STATE,
    /// User request
    MS_PROXY_ADV_UPD_REASON_USER,
    /// Peer request
    MS_PROXY_ADV_UPD_REASON_PEER,
    /// Started due to provisioning using PB-GATT
    MS_PROXY_ADV_UPD_REASON_PROV,
    /// Disconnection
    MS_PROXY_ADV_UPD_REASON_DISC,
};

/// State of the provisioning
enum mesh_prov_state
{
    /// Provisioning started - procedure started by a provisioner
    MS_PROV_STARTED,
    /// Provisioning succeed
    MS_PROV_SUCCEED,
    /// Provisioning failed
    MS_PROV_FAILED,
};

/// Authentication Method field values
enum mesh_prov_auth_method
{
    /// No OOB authentication is used
    MS_PROV_AUTH_NO_OOB,
    /// Static OOB authentication is used
    MS_PROV_AUTH_STATIC_OOB,
    /// Output OOB authentication is used
    MS_PROV_AUTH_OUTPUT_OOB,
    /// Input OOB authentication is used
    MS_PROV_AUTH_INPUT_OOB,
};

/// Output OOB Action field values
enum mesh_prov_out_oob
{
    /// Bit[0]: Blink
    MS_PROV_OUT_OOB_BLINK               = 0x0001,
    /// Bit[1]: Beep
    MS_PROV_OUT_OOB_BEEP                = 0x0002,
    /// Bit[2]: Vibrate
    MS_PROV_OUT_OOB_VIBRATE             = 0x0004,
    /// Bit[3]: Output Numeric
    MS_PROV_OUT_OOB_NUMERIC             = 0x0008,
    /// Bit[4]: Output Alphanumeric
    MS_PROV_OUT_OOB_ALPHANUMERIC        = 0x0010,
    /// Bit[5-15]: Reserved for Future Use
};

/// Input OOB Action field values
enum mesh_prov_in_oob
{
    /// Bit[0]: Push
    MS_PROV_IN_OOB_PUSH                 = 0x0001,
    /// Bit[1]: Twist
    MS_PROV_IN_OOB_TWIST                = 0x0002,
    /// Bit[2]: Input Numeric
    MS_PROV_IN_OOB_NUMERIC              = 0x0004,
    /// Bit[3]: Input Alphanumeric
    MS_PROV_IN_OOB_ALPHANUMERIC         = 0x0008,
    /// Bit[4-15]: Reserved for Future Use
};

enum mesh_prov_info
{
    /// URI Hash present or not in the unprovisioned device beacon
    MS_PROV_INFO_URI_HASH_PRESENT       = (1 << 0),
};

/*
 * TYPES
 ****************************************************************************************
 */

/// Local Identifier
typedef uint8_t m_lid_t;


/// @} MESH_DEFINES

#endif /* _MESH_DEF_H_ */
