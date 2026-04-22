/**
 ****************************************************************************************
 *
 * @file gatt.h
 *
 * @brief Generic Attribute Profile Messages.
 *
 ****************************************************************************************
 */

#ifndef _GATT_H_
#define _GATT_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include <stdint.h>
#include <stdbool.h>
#include "le_err.h"
#include "task.h"
#include "att.h"
#include "gap.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Macro for Instance of GATT TASK
#define GATT(idx)                        TASK_ID(GATT, idx)

/// GATT Task messages
/*@TRACE*/
enum gatt_msg_id
{
    /* Default event */
    /// Command Complete event
    GATT_CMP_EVT                       = MSG_ID(GATT, 0x00),

    /* ATTRIBUTE CLIENT */
    GATT_CLIENT_CMD                    = MSG_ID(GATT, 0x01),
    /// Server configuration request
    GATT_EXC_MTU_CMD                   = GATT_CLIENT_CMD, //MSG_ID(GATT, 0x01),
    /*Discover All Services */
    /*Discover Services by Service UUID*/
    /*Find Included Services*/
    /*Discover Characteristics by UUID*/
    /*Discover All Characteristics of a Service*/
    /*Discover All Characteristic Descriptors*/
    /// Discovery command
    GATT_DISC_CMD                      = GATT_CLIENT_CMD, //MSG_ID(GATT, 0x03),
    /*Read Value*/
    /*Read Using UUID*/
    /*Read Long Value*/
    /*Read Multiple Values*/
    /// Read command
    GATT_READ_CMD                      = GATT_CLIENT_CMD, //MSG_ID(GATT, 0x08),
    /*Write without response*/
    /*Write without response with Authentication*/
    /*Write Characteristic Value*/
    /*Signed Write Characteristic Value*/
    /*Write Long Characteristic Value*/
    /*Characteristic Value Reliable Write*/
    /*Write Characteristic Descriptors*/
    /*Write Long Characteristic Descriptors*/
    /*Characteristic Value Reliable Write*/
    /// Write command request
    GATT_WRITE_CMD                     = GATT_CLIENT_CMD, //MSG_ID(GATT, 0x0A),
    /* Cancel / Execute pending write operations */
    /// Execute write characteristic request
    GATT_EXECUTE_WRITE_CMD             = GATT_CLIENT_CMD, //MSG_ID(GATT, 0x0B),

    /// Indicate that the ATT MTU has been updated (negotiated)
    GATT_MTU_CHANGED_IND               = MSG_ID(GATT, 0x02),

    /* GATT -> HL: Events to Upper layer */
    /*Discover All Services*/
    /// Discovery services indication
    GATT_DISC_SVC_IND                  = MSG_ID(GATT, 0x04),
    /*Find Included Services*/
    /// Discover included services indication
    GATT_DISC_SVC_INCL_IND             = MSG_ID(GATT, 0x05),
    /*Discover All Characteristics of a Service*/
    /// Discover characteristic indication
    GATT_DISC_CHAR_IND                 = MSG_ID(GATT, 0x06),
    /*Discover All Characteristic Descriptors*/
    /// Discovery characteristic descriptor indication
    GATT_DISC_CHAR_DESC_IND            = MSG_ID(GATT, 0x07),

    /// Read response
    GATT_READ_IND                      = MSG_ID(GATT, 0x09),

    /* Reception of an indication or notification from peer device. */
    /// peer device triggers an event (notification)
    GATT_EVENT_IND                     = MSG_ID(GATT, 0x0C),
    /// peer device triggers an event that requires a confirmation (indication)
    GATT_EVENT_REQ_IND                 = MSG_ID(GATT, 0x0D),
    /// Confirm reception of event (trigger a confirmation message)
    GATT_EVENT_CFM                     = MSG_ID(GATT, 0x0E),

    /// Registration to peer device events (Indication/Notification).
    GATT_REG_TO_PEER_EVT_CMD           = MSG_ID(GATT, 0x0F),

    /* -------------------------- ATTRIBUTE SERVER ------------------------------- */
    /*Notify Characteristic*/
    /*Indicate Characteristic*/
    /// send an event to peer device
    GATT_SEND_EVT_CMD                  = MSG_ID(GATT, 0x10),
    GATT_SERVER_CFM                    = MSG_ID(GATT, 0x11),
    /// REad command confirmation from upper layers.
    GATT_READ_CFM                      = GATT_SERVER_CFM, //MSG_ID(GATT, 0x14),
    /// Write command confirmation from upper layers.
    GATT_WRITE_CFM                     = GATT_SERVER_CFM, //= MSG_ID(GATT, 0x16),
    /// Attribute info from upper layer confirmation
    GATT_ATT_INFO_CFM                  = GATT_SERVER_CFM, //= MSG_ID(GATT, 0x18),

    /* Indicate that read operation is requested. */
    /// Read command indicated to upper layers.
    GATT_READ_REQ_IND                  = MSG_ID(GATT, 0x13),

    /* Indicate that write operation is requested. */
    /// Write command indicated to upper layers.
    GATT_WRITE_REQ_IND                 = MSG_ID(GATT, 0x15),

    /* Indicate that write operation is requested. */
    /// Request Attribute info to upper layer - could be trigger during prepare write
    GATT_ATT_INFO_REQ_IND              = MSG_ID(GATT, 0x17),

    /* ----------------------- SERVICE DISCOVERY PROCEDURE  ---------------------- */
    /// Service Discovery command
    GATT_SDP_SVC_DISC_CMD              = MSG_ID(GATT, 0x19),
    /// Service Discovery indicate that a service has been found.
    GATT_SDP_SVC_IND                   = MSG_ID(GATT, 0x1A),

    /* -------------------------- TRANSACTION ERROR EVENT ------------------------ */
    /// Transaction Timeout Error Event no more transaction will be accepted
    GATT_TRANSACTION_TO_ERROR_IND      = MSG_ID(GATT, 0x1B),
    /// Indication to the task that sends the unknown message
    GATT_UNKNOWN_MSG_IND               = MSG_ID(GATT, 0x1C),

    /* --------------------------------- ROBUST CACHE --------------------------------- */
    /// Command used to read peer database hash
    GATT_READ_DB_HASH_CMD              = MSG_ID(GATT, 0x20),
    /// Provide value of the peer database hash
    GATT_DB_HASH_IND                   = MSG_ID(GATT, 0x21),
    /// Command used to enable Robust database caching
    /// (Register to service changed, Set client supported features and read database hash)
    GATT_ROBUST_DB_CACHE_EN_CMD        = MSG_ID(GATT, 0x22),
    /// Informs that peer database updated and need to be refreshed
    /// This also means that no more indication/notification will be received till database hash isn't read
    GATT_DB_CACHE_OUT_OF_SYNC_IND      = MSG_ID(GATT, 0x23),
    /// Informs that peer device database updated using service changed indication
    GATT_SVC_CHG_REQ_IND               = MSG_ID(GATT, 0x24),
    /// Message used to confirm that database updated is understood by application
    GATT_SVC_CHG_CFM                   = MSG_ID(GATT, 0x25),

    /* ----------------------------------- GATT INFO ---------------------------------- */
    /// Provide information about GATT for current connection that can be reuse on another connection
    GATT_CON_INFO_IND                  = MSG_ID(GATT, 0x28),
};


/// request operation type - application interface
/*@TRACE*/
enum gatt_operation
{
    /*              Attribute Client Flags              */
    /* No Operation (if nothing has been requested)     */
    /* ************************************************ */
    /// No operation
    GATT_NO_OP                                    = 0x00,

    /* Operation flags for MTU Exchange                 */
    /* ************************************************ */
    /// Perform MTU exchange
    GATT_MTU_EXCH                                 = 0x01,

    /* Operation flags for discovery operation          */
    /* ************************************************ */
    /// Discover all services
    GATT_DISC_ALL_SVC                             = 0x02,
    /// Discover services by UUID
    GATT_DISC_BY_UUID_SVC                         = 0x03,
    /// Discover included services
    GATT_DISC_INCLUDED_SVC                        = 0x04,
    /// Discover all characteristics
    GATT_DISC_ALL_CHAR                            = 0x05,
    /// Discover characteristic by UUID
    GATT_DISC_BY_UUID_CHAR                        = 0x06,
    /// Discover characteristic descriptor
    GATT_DISC_DESC_CHAR                           = 0x07,

    /* Operation flags for reading attributes           */
    /* ************************************************ */
    /// Read attribute
    GATT_READ                                     = 0x08,
    /// Read long attribute
    GATT_READ_LONG                                = 0x09,
    /// Read attribute by UUID
    GATT_READ_BY_UUID                             = 0x0A,
    /// Read multiple attribute
    GATT_READ_MULTIPLE                            = 0x0B,

    /* Operation flags for writing/modifying attributes */
    /* ************************************************ */
    /// Write attribute
    GATT_WRITE                                    = 0x0C,
    /// Write no response
    GATT_WRITE_NO_RESPONSE                        = 0x0D,
    /// Write signed
    GATT_WRITE_SIGNED                             = 0x0E,
    /// Execute write
    GATT_EXEC_WRITE                               = 0x0F,

    /* Operation flags for registering to peer device   */
    /* ************************************************ */
    /// Register to peer device events
    GATT_REGISTER                                 = 0x10,
    /// Unregister from peer device events
    GATT_UNREGISTER                               = 0x11,

    /* Operation flags for sending events to peer device*/
    /* ************************************************ */
    /// Send an attribute notification
    GATT_NOTIFY                                   = 0x12,
    /// Send an attribute indication
    GATT_INDICATE                                 = 0x13,
    /// Send a service changed indication
    GATT_SVC_CHANGED                              = 0x14,

    /* Service Discovery Procedure                      */
    /* ************************************************ */
    /// Search specific service
    GATT_SDP_DISC_SVC                             = 0x15,
    /// Search for all services
    GATT_SDP_DISC_SVC_ALL                         = 0x16,
    /// Cancel Service Discovery Procedure
    GATT_SDP_DISC_CANCEL                          = 0x17,

    /* Robust Cache                                     */
    /* ************************************************ */
    /// Read peer database hash
    GATT_READ_DB_HASH                             = 0x20,
    /// Enable Robust database cache feature
    GATT_ROBUST_DB_CACHE_EN                       = 0x21,

    GATT_READ_RSP                                 = 0x40,
    GATT_WRITE_RSP                                = 0x41,
    GATT_ATT_INFO_RSP                             = 0x42,
};

/// Service Discovery Attribute type
/*@TRACE*/
enum gatt_sdp_att_type
{
    /// No Attribute Information
    GATT_SDP_NONE,
    /// Included Service Information
    GATT_SDP_INC_SVC,
    /// Characteristic Declaration
    GATT_SDP_ATT_CHAR,
    /// Attribute Value
    GATT_SDP_ATT_VAL,
    /// Attribute Descriptor
    GATT_SDP_ATT_DESC,
};

/// Command complete event data structure
struct gatt_op_cmd
{
    /// GATT request type
    uint8_t operation;
    /// operation sequence number
    uint16_t seq_num;
};

/// Command complete event data structure
/*@TRACE*/
struct gatt_cmp_evt
{
    /// GATT request type
    uint8_t operation;
    /// Status of the request
    uint8_t status;
    /// operation sequence number - provided when operation is started
    uint16_t seq_num;
};


/// Service Discovery Command Structure
/*@TRACE*/
struct gatt_exc_mtu_cmd
{
    /// GATT request type
    uint8_t operation;
    /// operation sequence number
    uint16_t seq_num;
};

/// Indicate that the ATT MTU has been updated (negotiated)
/*@TRACE*/
struct gatt_mtu_changed_ind
{
    /// Exchanged MTU value
    uint16_t mtu;
    /// operation sequence number
    uint16_t seq_num;
};

/// Service Discovery Command Structure
/*@TRACE*/
struct gatt_disc_cmd
{
    /// GATT request type
    uint8_t  operation;
    /// UUID length
    uint8_t  uuid_len;
    /// operation sequence number
    uint16_t seq_num;
    /// start handle range
    uint16_t start_hdl;
    /// start handle range
    uint16_t end_hdl;
    /// UUID
    uint8_t  uuid[];
};


/// Discover Service indication Structure
/*@TRACE*/
struct gatt_disc_svc_ind
{
    /// start handle
    uint16_t start_hdl;
    /// end handle
    uint16_t end_hdl;
    /// UUID length
    uint8_t  uuid_len;
    /// service UUID
    uint8_t  uuid[];
};

/// Discover Service indication Structure
/*@TRACE*/
struct gatt_disc_svc_incl_ind
{
    /// element handle
    uint16_t attr_hdl;
    /// start handle
    uint16_t start_hdl;
    /// end handle
    uint16_t end_hdl;
    /// UUID length
    uint8_t uuid_len;
    /// included service UUID
    uint8_t uuid[];
};

/// Discovery All Characteristic indication Structure
/*@TRACE*/
struct gatt_disc_char_ind
{
    /// database element handle
    uint16_t attr_hdl;
    /// pointer attribute handle to UUID
    uint16_t pointer_hdl;
    /// properties
    uint8_t prop;
    /// UUID length
    uint8_t uuid_len;
    /// characteristic UUID
    uint8_t uuid[];
};

/// Discovery Characteristic Descriptor indication Structure
/*@TRACE*/
struct gatt_disc_char_desc_ind
{
    /// database element handle
    uint16_t attr_hdl;
    /// UUID length
    uint8_t uuid_len;
    /// Descriptor UUID
    uint8_t uuid[];
};


/// Simple Read (GATT_READ or GATT_READ_LONG)
/*@TRACE
 gatt_read = gatt_read_simple
 gatt_read_long = gatt_read_simple*/
struct gatt_read_simple
{
    /// attribute handle
    uint16_t handle;
    /// start offset in data payload
    uint16_t offset;
    /// Length of data to read (0 = read all)
    uint16_t length;
};

/// Read by UUID: search UUID and read it's characteristic value (GATT_READ_BY_UUID)
/// Note: it doesn't perform an automatic read long.
/*@TRACE*/
struct gatt_read_by_uuid
{
    /// Start handle
    uint16_t start_hdl;
    /// End handle
    uint16_t end_hdl;
    /// Size of UUID
    uint8_t uuid_len;
    /// UUID value
    uint8_t uuid[];
};

/// Read Multiple short characteristic (GATT_READ_MULTIPLE)
/*@TRACE*/
struct gatt_read_multiple
{
    /// attribute handle
    uint16_t handle;
    /// Known Handle length (shall be != 0)
    uint16_t len;
};

/// request union according to read type
/*@TRACE
 @trc_ref gatt_operation
 */
union gatt_read_req
{
    /// Simple Read (GATT_READ or GATT_READ_LONG)
    //@trc_union parent.operation == GATT_READ or parent.operation == GATT_READ_LONG
    struct gatt_read_simple simple;
    /// Read by UUID (GATT_READ_BY_UUID)
    //@trc_union parent.operation == GATT_READ_BY_UUID
    struct gatt_read_by_uuid by_uuid;
    /// Read Multiple short characteristic (GATT_READ_MULTIPLE)
    //@trc_union parent.operation == GATT_READ_MULTIPLE
    struct gatt_read_multiple multiple[1];
};

/// Read command (Simple, Long, Multiple, or by UUID)
/*@TRACE*/
struct gatt_read_cmd
{
    /// request type
    uint8_t operation;
    /// number of read (only used for multiple read)
    uint8_t nb;
    /// operation sequence number
    uint16_t seq_num;
    /// request union according to read type
    union gatt_read_req req;
};

/// Attribute value read indication
/*@TRACE*/
struct gatt_read_ind
{
    /// Attribute handle
    uint16_t handle;
    /// Read offset
    uint16_t offset;
    /// Read length
    uint16_t length;
    /// Handle value
    uint8_t value[];
};

/// Write peer attribute value command
/*@TRACE*/
struct gatt_write_cmd
{
    /// Request type
    uint8_t operation;
    /// Perform automatic execution
    /// (if false, an ATT Prepare Write will be used this shall be use for reliable write)
    bool auto_execute;
    /// operation sequence number
    uint16_t seq_num;
    /// Attribute handle
    uint16_t handle;
    /// Write offset
    uint16_t offset;
    /// Write length
    uint16_t length;
    /// Internal write cursor shall be initialized to 0
    uint16_t cursor;
    /// Value to write
    uint8_t value[];
};

/// Write peer attribute value command
/*@TRACE*/
struct gatt_execute_write_cmd
{
    /// Request type
    uint8_t operation;

    /// [True = perform/False cancel] pending write operations
    bool execute;
    /// operation sequence number
    uint16_t seq_num;
};
/// peer device triggers an event (notification)
/*@TRACE*/
struct gatt_event_ind
{
    /// Event Type
    uint8_t type;
    /// Data length
    uint16_t length;
    /// Attribute handle
    uint16_t handle;
    /// Event Value
    uint8_t value[];
};

/// peer device triggers an event that requires a confirmation (indication)
/*@TRACE*/
struct gatt_event_req_ind
{
    /// Event Type
    uint8_t type;
    /// Data length
    uint16_t length;
    /// Attribute handle
    uint16_t handle;
    /// Event Value
    uint8_t value[];
};

/// Confirm reception of event (trigger a confirmation message)
/*@TRACE*/
struct gatt_event_cfm
{
    /// Attribute handle
    uint16_t handle;
};

/// Register to peer device events command
/*@TRACE*/
struct gatt_reg_to_peer_evt_cmd
{
    /// Request type
    uint8_t operation;
    /// operation sequence number
    uint16_t seq_num;
    /// attribute start handle
    uint16_t start_hdl;
    /// attribute end handle
    uint16_t end_hdl;
};

/// Send an event to peer device
/*@TRACE*/
struct gatt_send_evt_cmd
{
    /// Request type (notification / indication)
    uint8_t operation;
    /// operation sequence number
    uint16_t seq_num;
    /// characteristic handle
    uint16_t handle;
    /// length of packet to send
    uint16_t length;
    /// data value
    uint8_t  value[];
};

/// Inform that attribute value is requested by lower layers.
/*@TRACE*/
struct gatt_read_req_ind
{
    /// Handle of the attribute that has to be read
    uint16_t handle;
};

/// Confirm Read Request requested by GATT to profile
/*@TRACE*/
struct gatt_read_cfm
{
    /// Response type
    uint8_t operation;
    /// Status of read command execution by upper layers
    uint8_t status;
    /// Handle of the attribute read
    uint16_t handle;
    /// Data length read
    uint16_t length;
    /// attribute data value
    uint8_t  value[];
};

/// Inform that a modification of database has been requested by peer device.
/*@TRACE*/
struct gatt_write_req_ind
{
    /// Handle of the attribute that has to be written
    uint16_t handle;
    /// offset at which the data has to be written
    uint16_t offset;
    /// Data length to be written
    uint16_t length;
    /// Data to be written in attribute database
    uint8_t  value[];
};

/// Confirm modification of database from upper layer when requested by peer device.
/*@TRACE*/
struct gatt_write_cfm
{
    /// Response type
    uint8_t operation;
    /// Status of write command execution by upper layers
    uint8_t status;
    /// Handle of the attribute written
    uint16_t handle;
};

/// Parameters for @ref GATT_SEND_SVC_CHANGED_CMD message
/*@TRACE*/
struct gatt_send_svc_changed_cmd
{
    /// Request Type
    uint8_t operation;
    /// operation sequence number
    uint16_t seq_num;
    /// Start of Affected Attribute Handle Range
    uint16_t svc_shdl;
    /// End of Affected Attribute Handle Range
    uint16_t svc_ehdl;
};

/// Parameters for @ref GATT_SVC_CHANGED_CFG_IND and @ref GATT_SVC_CHANGED message
/*@TRACE*/
struct gatt_svc_changed_cfg
{
    /**
     * Current value of the Client Characteristic Configuration descriptor for the Service
     * Changed characteristic
     */
    uint16_t ind_cfg;
};


/// Request Attribute info to upper layer - could be trigger during prepare write
/*@TRACE*/
struct gatt_att_info_req_ind
{
    /// Handle of the attribute for which info are requested
    uint16_t handle;
};

/// Attribute info from upper layer confirmation
/*@TRACE*/
struct gatt_att_info_cfm
{
    /// Response type
    uint8_t operation;
    /// use to know if it's possible to modify the attribute
    /// can contains authorization or application error code.
    uint8_t  status;
    /// Handle of the attribute
    uint16_t handle;
    /// Current length of the attribute
    uint16_t length;
};


/// Service Discovery command
/*@TRACE*/
struct gatt_sdp_svc_disc_cmd
{
    /// GATT Request Type
    /// - GATT_SDP_DISC_SVC Search specific service
    /// - GATT_SDP_DISC_SVC_ALL Search for all services
    /// - GATT_SDP_DISC_CANCEL Cancel Service Discovery Procedure
    uint8_t operation;
    /// Service UUID Length
    uint8_t  uuid_len;
    /// operation sequence number
    uint16_t seq_num;
    /// Search start handle
    uint16_t start_hdl;
    /// Search end handle
    uint16_t end_hdl;
    /// Service UUID
    uint8_t  uuid[ATT_UUID128_LEN];
};


/// Information about included service
/*@TRACE*/
struct gatt_sdp_include_svc
{
    /// Attribute Type
    /// - GATT_SDP_INC_SVC: Included Service Information
    uint8_t att_type;
    /// Included service UUID Length
    uint8_t uuid_len;
    /// Included Service UUID
    uint8_t  uuid[ATT_UUID128_LEN];
    /// Included service Start Handle
    uint16_t start_hdl;
    /// Included service End Handle
    uint16_t end_hdl;
};

/// Information about attribute characteristic
/*@TRACE*/
struct gatt_sdp_att_char
{
    /// Attribute Type
    /// - GATT_SDP_ATT_CHAR: Characteristic Declaration
    uint8_t att_type;
    /// Value property
    uint8_t prop;
    /// Value Handle
    uint16_t handle;
};

/// Information about attribute
/*@TRACE*/
struct gatt_sdp_att
{
    /// Attribute Type
    /// - GATT_SDP_ATT_VAL: Attribute Value
    /// - GATT_SDP_ATT_DESC: Attribute Descriptor
    uint8_t  att_type;
    /// Attribute UUID Length
    uint8_t  uuid_len;
    /// Attribute UUID
    uint8_t  uuid[ATT_UUID128_LEN];
};

/// Attribute information
/*@TRACE
 @trc_ref gatt_sdp_att_type
 */
union gatt_sdp_att_info
{
    /// Attribute Type
    uint8_t att_type;
    /// Information about attribute characteristic
    //@trc_union att_type == GATT_SDP_ATT_CHAR
    struct gatt_sdp_att_char att_char;
    /// Information about included service
    //@trc_union att_type == GATT_SDP_INC_SVC
    struct gatt_sdp_include_svc inc_svc;
    /// Information about attribute
    //@trc_union att_type == GATT_SDP_ATT_VAL or att_type == GATT_SDP_ATT_DESC
    struct gatt_sdp_att att;
};


/// Service Discovery indicate that a service has been found.
/*@TRACE
 @trc_arr info $end_hdl - $start_hdl
 */
struct gatt_sdp_svc_ind
{
    /// Service UUID Length
    uint8_t  uuid_len;
    /// Service UUID
    uint8_t  uuid[ATT_UUID128_LEN];
    /// Service start handle
    uint16_t start_hdl;
    /// Service end handle
    uint16_t end_hdl;
    /// attribute information present in the service
    /// (length = end_hdl - start_hdl)
    union gatt_sdp_att_info info[];
};

/// Indicate that an unknown message has been received
/*@TRACE*/
struct gatt_unknown_msg_ind
{
    /// Unknown message id
    msg_id_t unknown_msg_id;
};

/// Command used to enable Robust database caching
/// (Register to service changed, Set client supported features and read database hash)
/*@TRACE*/
struct gatt_robust_db_cache_en_cmd
{
    /// GATT Request Type
    /// - GATT_ROBUST_DB_CACHE_EN Enable Robust database caching
    uint8_t  operation;
    /// operation sequence number
    uint16_t seq_num;
};

/// Command used to read peer database hash
/*@TRACE*/
struct gatt_read_db_hash_cmd
{
    /// GATT Request Type
    /// - GATT_READ_DB_HASH Read peer database hash
    uint8_t  operation;
    /// operation sequence number
    uint16_t seq_num;
};

/// Provide value of the peer database hash
/*@TRACE*/
struct gatt_db_hash_ind
{
    /// Database Hash
    uint8_t hash[GAP_KEY_LEN];
};

/// Informs that peer device database updated using service changed indication
/*@TRACE
 @trc_arr info $end_hdl - $start_hdl
 */
struct gatt_svc_chg_req_ind
{
    /// Start handle
    uint16_t start_handle;
    /// End Handle
    uint16_t end_handle;
};

/// Provide information about GATT for current connection that can be reuse on another connection
/*@TRACE*/
struct gatt_con_info_ind
{
    // GATT Client Side
    /// Peer GATT Service Start handle
    uint16_t gatt_start_handle;
    /// Peer GATT Service End Handle
    uint16_t gatt_end_handle;
    /// Peer Service Change value handle
    uint16_t svc_chg_handle;

    // GATT Service Side
    /// Client bond data information (@see enum gapc_cli_info)
    uint8_t  cli_info;
    /// Client supported features    (@see enum gapc_cli_feat)
    uint8_t  cli_feat;
};

#endif // _GATT_H_
