/**
 ****************************************************************************************
 *
 * @file mesh_msg.h
 *
 * @brief Header file for Mesh Stack Message Application Programming Interface
 *
 ****************************************************************************************
 */

#ifndef MESH_MSG_H_
#define MESH_MSG_H_

/**
 ****************************************************************************************
 * @defgroup MESH_MSG Mesh Stack Message Application Programming Interface
 * @ingroup MESH
 * @brief  Mesh Stack Message Application Programming Interface
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include <stdint.h>
#include "task.h"
#include "mesh_def.h"


/*
 * MACROS
 ****************************************************************************************
 */

/// Reuse TASK_HCI(0x02) as Mesh Task ID
#define TASK_MESH                       0x02

/// Retrieve message value from its index - TID_MESH = 10 @see task.h
#define MESH_MSG_ID(id)                 MSG_ID(MESH, id)

/*
 * ENUMERATIONS FOR MESH STACK
 ****************************************************************************************
 */

/// Mesh Stack Message Indexes
enum mesh_msg_id
{
    /* *************************************************** */
    /* *                  MESH SERVICE                   * */
    /* *************************************************** */

    /// Mesh Command
    MESH_CMD                            = MESH_MSG_ID(0),
    /// Mesh Command Complete event
    MESH_CMP_EVT                        = MESH_MSG_ID(1),

    // Mesh service state change indication
    MESH_STATE_IND                      = MESH_MSG_ID(2),

    /// Inform/Get Registered Fault state or Request to start a test procedure
    MESH_FNDH_FAULT_IND                 = MESH_MSG_ID(3),
    /// Response containing current Registered Fault state for primary element
    MESH_FNDH_FAULT_CFM                 = MESH_MSG_ID(4),
    /// Inform about new publication period for Current Health state of primary element
    MESH_FNDH_PERIOD_IND                = MESH_MSG_ID(5),

    /// Mesh Provisioning Data Request
    MESH_PROV_DATA_REQ_IND              = MESH_MSG_ID(6),
    /// Response containing the required composition data page
    MESH_PROV_COMPO_DATA_CFM            = MESH_MSG_ID(7),
    /// Mesh Provisioning parameters response
    MESH_PROV_PARAM_DATA_CFM            = MESH_MSG_ID(8),
    /// Mesh Provisioning Authentication Data response
    MESH_PROV_AUTH_DATA_CFM             = MESH_MSG_ID(9),

    /// Mesh Friend Offer reception indication
    MESH_LPN_OFFER_IND                  = MESH_MSG_ID(10),

    /// Configuration loaded indication for application.
    MESH_CONF_LOAD_IND                  = MESH_MSG_ID(11),
    /// Configuration update indication for Storage.
    MESH_CONF_UPDATE_IND                = MESH_MSG_ID(12),

    /* *************************************************** */
    /* *                  MESH MODEL                     * */
    /* *************************************************** */

    /// Mesh Model Command
    MESH_MDL_MSG_FIRST                  = MESH_MSG_ID(100),
    MESH_MDL_CMD                        = MESH_MDL_MSG_FIRST,
    /// Mesh Model Command Complete Event
    MESH_MDL_CMP_EVT                    = MESH_MSG_ID(101),
    /// Mesh Model Indication
    MESH_MDL_IND                        = MESH_MSG_ID(102),
    /// Mesh Model Request Indication
    MESH_MDL_REQ_IND                    = MESH_MSG_ID(103),
    /// Mesh Model Confirm
    MESH_MDL_CFM                        = MESH_MSG_ID(104),
    MESH_MDL_MSG_LAST                   = MESH_MDL_CFM,
};

/*
 * ENUMERATIONS FOR MESH PROFILE
 ****************************************************************************************
 */

/// Indicate codes for MESH_STATE_IND message
enum ms_state_ind_code
{
    /// Mesh profile state change indication
    MS_PRF_STATE_IND,
    /// Mesh Provisioning state change indication
    MS_PROV_STATE_IND,
    /// Mesh Proxy advertising state update indication
    MS_PROXY_ADV_UPD_IND,
    /// Inform about updated attention state
    MS_ATEN_UPD_IND,
    /// Inform about requested node reset
    MS_NODE_RST_IND,
};

/// Indicate codes for MESH_FNDH_FAULT_IND message
enum ms_fault_ind_code
{
    /// Inform that clear of Registered Fault state identified by Company ID has been received
    MS_FAULT_CLEAR_IND,
    /// Request to get the current Registered Fault state identified by Company ID
    MS_FAULT_GET_REQ_IND,
    /// Request to start a test procedure of primary element
    MS_FAULT_TEST_REQ_IND,
    /// Test procedure, MESH_FAULT_CFM message isn't expected
    MS_FAULT_TEST_NAK_IND,
};

/// Indicate codes for MESH_PROV_DATA_REQ_IND message
enum mesh_prov_data_req_code
{
    /// Request Provisioning Authentication Data (@see enum mesh_prov_auth_method)
    MS_PROV_AUTH_DATA_REQ_IND           = 0,
    // No OOB authentication is used
    MS_PROV_AUTH_NO_OOB_REQ_IND         = MS_PROV_AUTH_DATA_REQ_IND,
    // Static OOB authentication is used
    MS_PROV_AUTH_STATIC_OOB_REQ_IND,
    // Output OOB authentication is used
    MS_PROV_AUTH_OUTPUT_OOB_REQ_IND,
    // Input OOB authentication is used
    MS_PROV_AUTH_INPUT_OOB_REQ_IND,

    /// Request Provisioning Parameters
    MS_PROV_PARAM_DATA_REQ_IND,

    /// Request a page of the composition data
    MS_PROV_COMPO_DATA_REQ_IND,
};

/// Indicate codes for MESH_CONF_LOAD_IND and MESH_CONF_UPDATE_IND message
enum ms_conf_type
{
    /// Mesh Created to load
    MS_CONF_TYPE_CREATE                 = 0,

    /// Mesh State updated
    MS_CONF_TYPE_STATE,

    /// Network key updated or deleted
    MS_CONF_TYPE_NET_KEY,

    /// Application key updated or deleted
    MS_CONF_TYPE_APP_KEY,

    /// Model publication parameters updated
    MS_CONF_TYPE_PUBLI_PARAM,
    /// Model subscription list updated
    MS_CONF_TYPE_SUBS_LIST,
    /// Model/application key binding updated
    MS_CONF_TYPE_BINDING,

    /// Friendship with LPN updated or lost
    MS_CONF_TYPE_LPN,

    /// Friendship with Friend updated or lost
    MS_CONF_TYPE_FRIEND,
};

/// State updated index for MS_CONF_TYPE_STATE
enum ms_conf_state_index
{
     /// Primary element unicast address
     MS_CONF_STATE_UNICAST_ADDR         = 0,
     /// Default TTL state
     MS_CONF_STATE_DEFAULT_TTL,
     /// Secure network beacon state
     MS_CONF_STATE_BEACON,
     /// Network transmit state
     MS_CONF_STATE_NET_TX,
     /// Relay state
     MS_CONF_STATE_RELAY,
     /// GATT proxy state
     MS_CONF_STATE_GATT_PROXY,
     /// Friend state
     MS_CONF_STATE_FRIEND,
     /// Device key
     MS_CONF_STATE_DEV_KEY,
     /// IV and SEQ values
     MS_CONF_STATE_IV_SEQ,
};

/// Status updated for MS_CONF_TYPE_SUBS_LIST
enum ms_conf_mdl_subs_status
{
    // Model subscription add
    MS_CONF_MDL_SUBS_ADD,
    // Model subscription delete
    MS_CONF_MDL_SUBS_DEL,
    // Model subscription delete all
    MS_CONF_MDL_SUBS_DEL_ALL,
    // Model subscription virtual address add
    MS_CONF_MDL_SUBS_VADDR_ADD,
    // Model subscription virtual address delete
    MS_CONF_MDL_SUBS_VADDR_DEL,
};

/*
 * MESSAGE DEFINITIONS FOR MESH SERVICE
 ****************************************************************************************
 */

/// Command required structure (without parameters)
typedef struct ms_cmd
{
    /// Command code (@see enum ms_cmd_code)
    uint8_t     cmd_code;
} ms_cmd_t;

/// Command complete event required structure (without parameters)
typedef struct ms_cmp_evt
{
    /// Command code (@see enum ms_cmd_code)
    uint8_t     cmd_code;
    /// Status of the command execution
    uint8_t     status;
} ms_cmp_evt_t;

/// State indication common structure
typedef struct ms_state_ind
{
    /// Indication code (@see enum ms_state_ind_code)
    uint8_t     ind_code;
    /// State
    uint8_t     state;
    /// Status or Reason
    uint16_t    status;
} ms_state_ind_t;

/// Mesh Profile state change indication
typedef struct ms_prf_state_ind
{
    /// Indication code - MS_PRF_STATE_IND
    uint8_t     ind_code;
    /// Enabled or Disabled state (@see enum m_prf_state)
    uint8_t     prf_state;
    /// Status, success with MESH_ERR_NO_ERROR
    uint16_t    status;
} ms_prf_state_ind_t;

/// Mesh Provisioning state change indication
typedef struct ms_prov_state_ind
{
    /// Indication code - MS_PROV_STATE_IND
    uint8_t     ind_code;
    /// Provisioning procedure state (@see enum mesh_prov_state)
    uint8_t     prov_state;
    /// Relevant only for provisioning failed (failed reason)
    uint16_t    status;
} ms_prov_state_ind_t;

/// Indicate that proxy advertising has been started or stopped and the reason
typedef struct ms_proxy_adv_upd_ind
{
    /// Indication code - MS_PROXY_ADV_UPD_IND
    uint8_t     ind_code;
    /// State (@see enum mesh_proxy_adv_upd)
    uint8_t     upd_state;
    /// Reason (@see enum mesh_proxy_adv_upd_reason)
    uint16_t    reason;
} ms_proxy_adv_upd_ind_t;

/// Inform about attention state update
typedef struct ms_aten_upd_ind
{
    /// Indication code - MS_ATEN_UPD_IND
    uint8_t     ind_code;
    /// Attention state
    uint8_t     aten_state;
    /// Status reserved
    uint16_t    status;
} ms_aten_upd_ind_t;

/// Inform about requested node reset
typedef struct ms_node_rst_ind
{
    /// Indication code - MS_NODE_RST_IND
    uint8_t     ind_code;
    /// Handle value configured by model
    uint8_t     tx_hdl;
    /// Transmission status
    uint16_t    status;
} ms_node_rst_ind_t;

/// Configuration load indication message structure
typedef struct ms_conf_load_ind
{
    /// Load type (@see ms_conf_type)
    uint8_t     cfg_type;
    /// Local index
    uint8_t     index;
    /// Status
    uint16_t    status;
} ms_conf_load_ind_t;

/// Configuration update indication message structure
typedef struct ms_conf_update_ind
{
    /// Update type (@see ms_conf_type)
    uint8_t     upd_type;
    /// Local index or status
    uint8_t     value1;
    /// Entry value
    uint16_t    value2;
} ms_conf_update_ind_t;

/// Low Power Node offer reception indication structure
typedef struct ms_lpn_offer_ind
{
    /// Address of Friend node that sent the Friend Offer message
    uint16_t frd_addr;
    /// Friend Counter
    uint16_t frd_cnt;

    /// Friend Offer params
    union {
      struct {
        /// Receive window value supported by the Friend node
        uint8_t  rx_window;
        /// Queue size available on the Friend node
        uint8_t  queue_size;
        /// Size of the subscription list that can be supported by the Friend Node
        uint8_t  subs_list_size;
        /// RSSI measured by the Friend node
        int8_t   rssi;
      };
      uint32_t wd_offer;
    };
} ms_lpn_offer_ind_t;


/// Request to start a test procedure of primary element
typedef struct ms_fndh_fault_ind
{
    /// Indication code (@see enum ms_fault_ind_code)
    uint8_t     ind_code;
    /// Test ID, Relevant only for MS_FAULT_TEST_REQ_IND MS_FAULT_TEST_NAK_IND
    uint8_t     test_id;
    /// Company ID
    uint16_t    comp_id;
} ms_fndh_fault_ind_t;

/// Response containing current Registered Fault state for primary element
typedef struct ms_fndh_fault_cfm
{
    /// Accept (true) or reject (false) the request
    bool        accept;
    /// Test ID
    uint8_t     test_id;
    /// Company ID
    uint16_t    comp_id;
    /// Length of fault array
    uint8_t     length;
    /// Fault array
    uint8_t     fault_array[];
} ms_fndh_fault_cfm_t;

/// Inform about new publication period for Current Health state of primary element
typedef struct ms_fndh_period_ind
{
    /// Publication period in milliseconds when no fault is known
    uint32_t    period_ms;
    /// Publication period in milliseconds when one or several fault are known
    uint32_t    period_fault_ms;
} ms_fndh_period_ind_t;

/// Request Provisioning Parameters
typedef struct ms_prov_param_data_req_ind
{
    /// Request indication code - MS_PROV_PARAM_DATA_REQ_IND
    uint8_t     req_code;
} ms_prov_param_data_req_ind_t;

/// Request a page of the composition data
typedef struct ms_prov_compo_data_req_ind
{
    /// Request indication code - MS_PROV_COMPO_DATA_REQ_IND
    uint8_t     req_code;
    /// Page
    uint8_t     page;
} ms_prov_compo_data_req_ind_t;

/// Mesh Provisioning Authentication Data Request
typedef struct ms_prov_auth_data_req_ind
{
    /// Request indication code - MS_PROV_AUTH_DATA_REQ_IND
    ///   - Same index with Authentication method (@see enum mesh_prov_auth_method)
    uint8_t     auth_method;
    /// expected authentication maximum data size
    uint8_t     auth_size;
    /// Authentication Action:
    ///   - MS_PROV_AUTH_NO_OOB     = Prohibited
    ///   - MS_PROV_AUTH_STATIC_OOB = 16 bytes LSB static out of band data required
    ///   - MS_PROV_AUTH_OUTPUT_OOB = @see enum mesh_prov_out_oob, 1 bit set.
    ///   - MS_PROV_AUTH_INPUT_OOB  = @see enum mesh_prov_in_oob, 1 bit set.
    uint16_t    auth_action;
} ms_prov_auth_data_req_ind_t;

typedef struct ms_prov_data_req_ind
{
    /// Request indication code (@see enum mesh_prov_data_req_code)
    uint8_t     req_code;
    /// Param value1
    uint8_t     value1;
    /// Param value2
    uint16_t    value2;
} ms_prov_data_req_ind_t;

/// Mesh Provisioning Parameters Response
typedef struct ms_prov_param_cfm
{
    /// Device UUID
    uint8_t     dev_uuid[MESH_DEV_UUID_LEN];
    /// URI hash
    uint32_t    uri_hash;
    /// OOB information
    uint16_t    oob_info;
    /// Public key OOB information available
    uint8_t     pub_key_oob;
    /// Static OOB information available
    uint8_t     static_oob;
    /// Maximum size of Output OOB supported
    uint8_t     out_oob_size;
    /// Maximum size in octets of Input OOB supported
    uint8_t     in_oob_size;
    /// Supported Output OOB Actions (@see enum mesh_prov_out_oob)
    uint16_t    out_oob_action;
    /// Supported Input OOB Actions (@see enum mesh_prov_in_oob)
    uint16_t    in_oob_action;
    /// Bit field providing additional information (@see enum mesh_prov_info)
    uint8_t     info;
} ms_prov_param_cfm_t;

/// Response containing the required composition data page
typedef struct ms_compo_data_cfm
{
    /// Page
    uint8_t     page;
    /// Length
    uint8_t     length;
    /// Data
    uint8_t     data[];
} ms_compo_data_cfm_t;

/// Mesh Provisioning Authentication Data Response
typedef struct ms_prov_auth_data_cfm
{
    /// 1, Accept pairing request, 0 reject
    uint8_t     accept;
    /// Authentication data size (<= requested size else pairing automatically rejected)
    uint8_t     auth_size;
    /// Authentication data (LSB for a number or array of bytes)
    uint8_t     auth_data[];
} ms_prov_auth_data_cfm_t;


/*
 * ENUMERATIONS FOR MESH MODELS
 ****************************************************************************************
 */

/// Indication codes for MESH_MDL_IND message
enum mm_ind_code
{
    /// Model registered indication
    MM_REGISTER_IND             = 0,

    /// State update indication
    MM_SRV_STATE_UPD_IND        = 10,
    /// Set Generic Location state indication (global part) for Generic Location Server model
    MM_SRV_LOCG_UPD_IND,
    /// Set Generic Location state indication (local part) for Generic Location Server model
    MM_SRV_LOCL_UPD_IND,

    /// Received state indication for:
    ///    - Generic OnOff Client model
    ///    - Generic Default Transition Time Client model
    ///    - Generic Level Client model
    ///    - Generic Power OnOff Client model
    MM_CLI_STATE_IND            = 100,
    /// Received Generic Battery state indication for Generic Battery Client model
    MM_CLI_BAT_IND,
    /// Received Generic Location state indication (global part) for Generic Location Client model
    MM_CLI_LOCG_IND,
    /// Received Generic Location state indication (local part) for Generic Location Client model
    MM_CLI_LOCL_IND,
    /// Get a Generic Property state value or list of Generic Property states
    MM_CLI_PROP_IND,
    /// Received list of User or Admin or Manufacturer properties
    MM_CLI_PROP_LIST_IND,

    /// Group event indication
    MM_GRP_EVENT_IND            = 200,
    /// State set by main model indication
    MM_GRP_STATE_IND,
};

/// Indication codes for MESH_MDL_REQ_IND message
enum mm_req_code
{
    /// Request battery information for a given element
    MM_SRV_BAT_REQ_IND          = 0,
    /// Request Generic Location state for a given element (global part)
    MM_SRV_LOCG_REQ_IND,
    /// Request Generic Location state for a given element (local part)
    MM_SRV_LOCL_REQ_IND,

    /// Request to get Generic Property value
    MM_SRV_PROP_GET_REQ_IND,
    /// Request to set Generic Property value
    MM_SRV_PROP_SET_REQ_IND,

    /// Request start of a new transition to the main model
    MM_GRP_TRANS_REQ_IND        = 50,
};

/*
 * MESSAGE DEFINITIONS FOR MESH MODELS
 ****************************************************************************************
 */

/// Minimal content for MESH_MDL_CMD message
typedef struct mm_cmd
{
    /// Command Code (depends on indicated API)
    uint8_t     cmd_code;
} mm_cmd_t;

/// Minimal content for MESH_MDL_CMP_EVT message
typedef struct mm_cmp_evt
{
    /// Command Code (depends on indicated API)
    uint8_t     cmd_code;
    /// Status of command execution
    uint8_t     status;
} mm_cmp_evt_t;

/// Minimal content for MESH_MDL_CFM message
typedef struct mm_cfm
{
    /// Request indication code (depends on indicated API)
    uint8_t     req_code;
    /// Status of command execution by application
    uint8_t     status;
} mm_cfm_t;

/// Common content for MESH_MDL_IND message
typedef struct mm_ind
{
    /// Indication code (@see enum mm_ind_code)
    uint8_t     ind_code;

    /* Model Register indication */

    /* Server Role indication */

    /* Client Role indication */
} mm_ind_t;

/// Model registered indication structure
typedef struct mm_register_ind
{
    /// Indication code - MM_REGISTER_IND
    uint8_t     ind_code;
    /// Model local index
    m_lid_t     mdl_lid;
    /// Element index
    uint8_t     elmt_idx;
    /// Model ID
    uint32_t    model_id;
} mm_register_ind_t;

/// State update indication structure
typedef struct mm_srv_state_upd_ind
{
    /// Indication code - MM_SRV_STATE_UPD_IND
    uint8_t     ind_code;
    /// Element index
    uint8_t     elmt_idx;

    /// State identifier (@see enum mm_state_id)
    uint16_t    state_id;
    /// New state value or targeted state value depending on transition time
    uint32_t    state;
    /// Transition time in milliseconds
    uint32_t    trans_time_ms;
} mm_srv_state_upd_ind_t;

/// Set Generic Location state (global part) indication structure
typedef struct mm_srv_locg_upd_ind
{
    /// Indication code - MM_SRV_LOCG_UPD_IND
    uint8_t     ind_code;
    /// Element index
    uint8_t     elmt_idx;

    /// Global Latitude
    int32_t     latitude;
    /// Global Longitude
    int32_t     longitude;
    /// Global Altitude
    int16_t     altitude;
} mm_srv_locg_upd_ind_t;

/// Set Generic Location state (local part) indication structure
typedef struct mm_srv_locl_upd_ind
{
    /// Indication code - MM_SRV_LOCL_UPD_IND
    uint8_t     ind_code;
    /// Element index
    uint8_t     elmt_idx;

    /// Local North
    int16_t     north;
    /// Local East
    int16_t     east;
    /// Local Altitude
    int16_t     altitude;
    /// Uncertainty
    uint16_t    uncertainty;
    /// Floor
    uint8_t     floor;
} mm_srv_locl_upd_ind_t;

/// Received state indication structure
typedef struct mm_cli_state_ind
{
    /// Indication code - MM_CLI_STATE_IND
    uint8_t     ind_code;
    /// Source address
    uint16_t    src;

    /// State Identifier (@see enum mm_state_id)
    uint16_t    state_id;
    /// State 1
    uint32_t    state_1;
    /// State 2
    uint32_t    state_2;
    /// Remaining time in milliseconds
    uint32_t    rem_time_ms;
} mm_cli_state_ind_t;

/// Received Generic Battery state indication structure
typedef struct mm_cli_bat_ind
{
    /// Indication code - MM_CLI_BAT_IND
    uint8_t     ind_code;
    /// Source address
    uint16_t    src;

    /// Time to discharge in minutes
    uint32_t    time_discharge;
    /// Time to charge in minutes
    uint32_t    time_charge;
    /// Battery level
    uint8_t     bat_lvl;
    /// Flags
    uint8_t     flags;
} mm_cli_bat_ind_t;

/// Received Generic Location state (global part) indication structure
typedef struct mm_cli_locg_ind
{
    /// Indication code - MM_CLI_LOCG_IND
    uint8_t     ind_code;
    /// Source address
    uint16_t    src;

    /// Global Latitude
    int32_t     latitude;
    /// Global Longitude
    int32_t     longitude;
    /// Global Altitude
    int16_t     altitude;
} mm_cli_locg_ind_t;

/// Received Generic Location state (local part) indication structure
typedef struct mm_cli_locl_ind
{
    /// Indication code - MM_CLI_LOCL_IND
    uint8_t     ind_code;
    /// Source address
    uint16_t    src;

    /// Local North
    int16_t     north;
    /// Local East
    int16_t     east;
    /// Local Altitude
    int16_t     altitude;
    /// Uncertainty
    uint16_t    uncertainty;
    /// Floor
    uint8_t     floor;
} mm_cli_locl_ind_t;

/// Received User or Admin or Manufacturer Property value indication structure
typedef struct mm_cli_prop_ind
{
    /// Indication code - MM_CLI_PROP_IND
    uint8_t     ind_code;
    /// Source address
    uint16_t    src;

    /// Property ID
    uint16_t    prop_id;
    /// Length
    uint16_t    length;
    /// User access
    uint8_t     user_access;
    /// Value
    uint8_t     val[];
} mm_cli_prop_ind_t;

/// Received list of User or Admin or Manufacturer properties indication structure
typedef struct mm_cli_prop_list_ind
{
    /// Indication code - MM_CLI_PROP_LIST_IND
    uint8_t     ind_code;
    /// Source address
    uint16_t    src;

    /// Number of properties
    uint16_t    nb_prop;
    /// Property type
    uint8_t     prop_type;
    /// Property IDs
    uint16_t    prop_ids[];
} mm_cli_prop_list_ind_t;

/// Common content for MESH_MDL_REQ_IND message
typedef struct mm_req_ind
{
    /// Request indication code (@see enum mm_req_code)
    uint8_t                     req_code;

    /* Server Role Request indication */
} mm_req_ind_t;

/// Get element state value request indication structure
typedef struct mm_srv_state_req_ind
{
    /// Request indication code (@see enum mm_req_code)
    uint8_t     req_code;
    /// Element index
    uint8_t     elmt_idx;
} mm_srv_state_req_ind_t;

/// Request to get Generic Property value
typedef struct mm_srv_prop_get_req_ind
{
    /// Request indication code - MM_SRV_PROP_GET_REQ_IND
    uint8_t     req_code;
    /// Element index
    uint8_t     elmt_idx;

    /// Property type
    uint8_t     prop_type;
    /// Property ID
    uint16_t    prop_id;
} mm_srv_prop_get_req_ind_t;

/// Request to set Generic Property value
typedef struct mm_srv_prop_set_req_ind
{
    /// Request indication code - MM_SRV_PROP_SET_REQ_IND
    uint8_t     req_code;
    /// Element index
    uint8_t     elmt_idx;

    /// Property type
    uint8_t     prop_type;
    /// Property ID
    uint16_t    prop_id;
    /// Value length
    uint16_t    length;
    /// Value
    uint8_t     val[];
} mm_srv_prop_set_req_ind_t;

/// @} MESH_MSG

#endif /* MESH_MSG_H_ */
