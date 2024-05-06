/**
 ****************************************************************************************
 *
 * @file mm_itf.h
 *
 * @brief Header file for Mesh Model Message Interface Definitions
 *
 ****************************************************************************************
 */

#ifndef _MM_ITF_H_
#define _MM_ITF_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include <stdint.h>
#include "string.h"
#include "mesh.h"
#include "mm_def.h"


/*
 * DEFINES
 ****************************************************************************************
 */

/// define the static keyword for this compiler
#ifndef __STATIC
    #define __STATIC                static
#endif

/// define the force inlining attribute for this compiler
#ifndef __INLINE__
    #define __INLINE__              __forceinline static
#endif


/*
 * ENUMERATIONS
 ****************************************************************************************
 */


/*
 * TYPE DEFINTIONS
 ****************************************************************************************
 */

typedef struct mm_mdl_env           mm_mdl_env_t;

typedef struct mm_route_env         mm_route_env_t;

/**
 ****************************************************************************************
 * @brief Inform model about reception of a mesh message with a supported opcode
 ****************************************************************************************
 */
typedef void (*mm_rx_cb)(mm_mdl_env_t *p_env, mesh_buf_t *p_buf, mm_route_env_t *p_route_env);

/**
 ****************************************************************************************
 * @brief Check if an opcode is handled by the model
 ****************************************************************************************
 */
typedef uint8_t (*mm_opcode_check_cb)(mm_mdl_env_t *p_env, uint32_t opcode);

/**
 ****************************************************************************************
 * @brief Set state value of a local Server Model
 ****************************************************************************************
 */
typedef uint8_t (*mm_srv_set_cb)(mm_mdl_env_t *p_env, uint16_t state_id, uint32_t state);

/**
 ****************************************************************************************
 * @brief Get state value using a Client Model
 ****************************************************************************************
 */
typedef uint8_t (*mm_cli_get_cb)(mm_mdl_env_t *p_env, m_lid_t app_key_lid, uint16_t dst,
                                    uint16_t get_info);

/**
 ****************************************************************************************
 * @brief Get state value using a Client Model
 ****************************************************************************************
 */
typedef uint8_t (*mm_cli_set_cb)(mm_mdl_env_t *p_env, m_lid_t app_key_lid, uint16_t dst,
                                    uint32_t state_1, uint32_t state_2, uint16_t set_info);

/**
 ****************************************************************************************
 * @brief Initiate transition to a new state value using a Client Model
 ****************************************************************************************
 */
typedef uint8_t (*mm_cli_trans_cb)(mm_mdl_env_t *p_env, m_lid_t app_key_lid, uint16_t dst,
                                    uint32_t state_1, uint32_t state_2,
                                    uint32_t trans_time_ms, uint16_t delay_ms,
                                    uint16_t trans_info);

/**
 ****************************************************************************************
 * @brief Definition of callback function called by the Binding Manager to inform the main model
 * when one of the model composing a model group has requested transition to a new state.
 *
 * @param[in] main_mdl_lid      Main model local index
 * @param[in] req_model_id      Model ID of model that has requested the transition
 * @param[in] trans_type        Transition type
 * @param[in] state_delta       State value or delta value
 ****************************************************************************************
 */
typedef void (*mm_bind_trans_req_cb) (m_lid_t main_mdl_lid, uint32_t req_model_id, 
                                    uint8_t trans_type, uint32_t state_delta);

/**
 ****************************************************************************************
 * @brief Definition of callback function called by the Binding Manager to inform a model about
 * transition or publication event
 *
 * @param[in] mdl_lid           Model local index
 * @param[in] event             Event (@see enum mesh_mdl_grp_event)
 ****************************************************************************************
 */
typedef void (*mm_bind_grp_event_cb) (m_lid_t mdl_lid, uint8_t event, uint8_t info);

/**
 ****************************************************************************************
 * @brief Definition of callback function by the Binding Manager to inform a model about
 * state value to be set
 *
 * @param[in] mdl_lid       Model local index
 * @param[in] type          State type (@see mm_state_type)
 * @param[in] state         State value
 ****************************************************************************************
 */
typedef void (*mm_bind_set_state_cb) (m_lid_t mdl_lid, uint8_t type, uint32_t state);


/*
 * STRUCTURES DEFINTIONS
 ****************************************************************************************
 */

/// Structure containing specific callback functions for a Server Model
typedef struct mm_srv_cb
{
    /// Set state value of a local Server Model
    mm_srv_set_cb       cb_set;
} mm_srv_cb_t;

/// Structure containing specific callback functions for a Client Model
typedef struct mm_cli_cb
{
    /// Get state value using a Client Model
    mm_cli_get_cb       cb_get;
    /// Set state value using a Client Model
    mm_cli_set_cb       cb_set;
    /// Initiate transition to a new state value using a Client Model
    mm_cli_trans_cb     cb_trans;
} mm_cli_cb_t;

/// Model Callback Structure
typedef struct mm_mdl_cb
{
    /// Inform model about reception of a mesh message with a supported opcode
    mm_rx_cb                cb_rx;
    /// Check if an opcode is handled by the model
    mm_opcode_check_cb      cb_opcode_check;
    /// Pointer to either callback functions for Server Model or Client Model
    union
    {
        /// Set state value of a local Server Model
        mm_srv_set_cb       cb_srv_set;
        /// Pointer to callback functions for Client Model
        const mm_cli_cb_t  *p_cb_cli;
    };
} mm_mdl_cb_t;

/// Basic structure for model information
struct mm_mdl_env
{
    /// Model ID
    uint32_t            model_id;
    /// Model callback functions
    mm_mdl_cb_t         mdl_cb;
    /// Information bit field (@see enum mm_info)
    uint8_t             info;
    /// Model Local Index
    m_lid_t             mdl_lid;
    /// Group Local Index - filled by Binding Manager
    m_lid_t             grp_lid;
    /// Element Index
    uint8_t             elmt_idx;
};

/// Basic structure for model information when sending of publications is supported
typedef struct mm_mdl_publi_env
{
    /// Basic model environment - Must be first element in the structure - DO NOT MOVE
    mm_mdl_env_t        env;
    /// Timer for sending of publications
    mesh_timer_t        tmr_publi;
    /// Publication period in milliseconds
    uint32_t            publi_period_ms;
} mm_mdl_publi_env_t;

/// Structure to allocate as part of model environment for replay protection
typedef struct mm_replay_env
{
    /// List of replay elements
    list_t              list_replay;
    /// Timer for replay protection
    mesh_timer_t        tmr_replay;
    /// Validity delay in milliseconds for replay elements inserted in the list
    uint16_t            delay_ms;
    /// Sum of delays of elements placed after the first replay element in the list
    uint16_t            delay_total_ms;
} mm_replay_env_t;

/// Buffer environment when handled by models
struct mm_route_env
{
    /// Opcode
    uint32_t            opcode;
    /// Information bitfield (@see enum mm_route_info)
    uint16_t            info;
    union
    {
        /// Source address (for RX)
        uint16_t        src;
        /// Destination address (for TX)
        uint16_t        dst;
    } u_addr;
    /// Application key local index
    m_lid_t             app_key_lid;
    /// Model local index
    m_lid_t             mdl_lid;
};


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/****************************************************************************************
 * APIs - Mesh Stack for Models
 ****************************************************************************************/

/**
 ****************************************************************************************
 * @brief Register a model instance.
 *
 * @param[in] model_id          Model ID.
 * @param[in] elmt_idx          Index of the element the model belongs to
 * @param[in] config            Configuration (@see mesh_mdl_config)
 *
 * @return The allocated Model LID if model instance has been registered
 *         MESH_INVALID_LID if it was not possible to register the model instance.
 ****************************************************************************************
 */
m_lid_t ms_register_model(uint32_t model_id, uint8_t elmt_idx, uint8_t config);

/**
 ****************************************************************************************
 * @brief Inform the Model State Manager about registration of a new model
 *
 * @param[in] model_id      Model Identifier
 * @param[in] elmt_idx      Index of element the model belongs to
 * @param[in] mdl_lid       Local index allocated by the mesh profile for the model
 * @param[in] mdl_role      Model Role
 * @param[in] env_size      Size of environment to allocate for this model
 *
 * @return Pointer to the new environment or NULL
 ****************************************************************************************
 */
mm_mdl_env_t* mm_state_register(uint8_t elmt_idx, uint32_t model_id, m_lid_t mdl_lid,
                              uint8_t mdl_role, uint16_t env_size);

/**
 ****************************************************************************************
 * @brief Get pointer to environment allocated for a given model
 *
 * @param[in] mdl_lid       Model local index
 *
 * @return Pointer to the environment
 ****************************************************************************************
 */
mm_mdl_env_t *mm_state_get_env(m_lid_t mdl_lid);

/**
 ****************************************************************************************
 * @brief Get local index allocated for a given model on a given element
 *
 * @param[in] elmt_idx      Element Index
 * @param[in] model_id      Model Identifier
 *
 * @return Model local index or MESH_INVALID_LID if not found.
 ****************************************************************************************
 */
m_lid_t mm_state_get_lid(uint8_t elmt_idx, uint32_t model_id);

/**
 ****************************************************************************************
 * @brief Convert a transition time value in milliseconds in its packed format
 *
 * @param[in] trans_time_ms       Transition time in milliseconds
 *
 * @return Packed transition time
 ****************************************************************************************
 */
uint8_t mm_get_trans_time(uint32_t trans_time_ms);

/**
 ****************************************************************************************
 * @brief Convert a transition time value in its packed format into a millisecond value
 *
 * @param[in] time_tt       Transition time (packed format)
 *
 * @return Transition time in milliseconds
 ****************************************************************************************
 */
uint32_t mm_get_trans_time_ms(uint8_t time_tt);

/**
 ****************************************************************************************
 * @brief Add a group of bound models
 *
 * @param[in] nb_mdl            Number of models bound with the main model
 * @param[in] elmt_idx          Index of element the models belong to
 * @param[out] p_grp_lid        Pointer to variable in which group local index will be returned
 * @param[in] main_mdl_lid      Local index of the main model
 * @param[in] cb_grp_event      Callback function for communication with the model when a group
 * event occurs
 * @param[in] cb_trans_req      Callback function used to inform the main model about a transition
 * request received by one of its bound model
 *
 * @return An error status
 ****************************************************************************************
 */
m_lid_t mm_bind_add_group(uint8_t nb_mdl, uint8_t elmt_idx, m_lid_t main_mdl_lid,
                              mm_bind_grp_event_cb cb_grp_event,
                              mm_bind_trans_req_cb cb_trans_req);

/**
 ****************************************************************************************
 * @brief Add a model to a group of bound models
 *
 * @param[in] grp_lid           Group local index
 * @param[in] mdl_lid           Local index of the model
 * @param[in] model_id          Model ID of the model
 * @param[in] cb_grp_event      Callback function for communication with the model when a group
 * event occurs
 *
 * @return An error status
 ****************************************************************************************
 */
uint8_t mm_bind_group_add_mdl(m_lid_t grp_lid, m_lid_t mdl_lid, uint32_t model_id,
                                  mm_bind_grp_event_cb cb_grp_event,
                                  mm_bind_set_state_cb cb_set_state);

/**
 ****************************************************************************************
 * @brief Set default transition time value for groups belonging to a given element
 *
 * @param[in] elmt_idx          Element index
 * @param[in] dft_trans_time    Default transition time value
 ****************************************************************************************
 */
void mm_bind_set_dft_trans_time(uint8_t elmt_idx, uint8_t dft_trans_time);

/**
 ****************************************************************************************
 * @brief Function used by a bound model to set state
 *
 * @param[in] grp_lid        Group local index
 * @param[in] type           State type (@see enum mm_state_type)
 * @param[in] model_id       Model ID of the model
 * @param[in] state          State value
 *
 * @return An error status (@see mesh_err)
 ****************************************************************************************
 */
uint8_t mm_bind_set_state(m_lid_t grp_lid, uint8_t type, uint32_t model_id, uint32_t state);

/**
 ****************************************************************************************
 * @brief Function used by a bound model to request a new transition.
 *
 * @param[in] grp_lid        Group local index
 * @param[in] mdl_lid        Model local index of model that requires a transition
 * @param[in] trans_type     Transition Type (@see enum mm_trans_type)
 * @param[in] state_delta    Either new state value or delta value to be applied
 * @param[in] trans_time     Transition Time (packed format)
 * @param[in] delay          Delay
 *
 * @return An error status (@see mesh_err)
 ****************************************************************************************
 */
uint8_t mm_bind_trans_req(m_lid_t grp_lid, m_lid_t mdl_lid,
                              uint8_t trans_type, uint32_t state_delta,  uint8_t trans_time,
                              uint8_t delay);

/**
 ****************************************************************************************
 * @brief Function called by a main model in order to initiate a new transition
 *
 * @param[in] grp_lid       Group local index
 * @param[in] trans_time    Transition time
 * @param[in] delay         Delay duration in multiple of 5ms
 *
 * @return An error status
 ****************************************************************************************
 */
uint8_t mm_bind_trans_new(m_lid_t grp_lid, uint8_t trans_type, uint8_t trans_time, uint8_t delay);

/**
 ****************************************************************************************
 * @brief Function called by a main model in order to reject a transition required by one of its
 * bound models
 *
 * @param[in] grp_lid       Group local index
 ****************************************************************************************
 */
void mm_bind_trans_reject(m_lid_t grp_lid);

/**
 ****************************************************************************************
 * @brief Function called by a main model once all target states have been set in order to start
 * the transition
 *
 * @param[in] grp_lid       Group local index
 *
 * @return An error status
 ****************************************************************************************
 */
uint8_t mm_bind_trans_start(m_lid_t grp_lid);

/**
 ****************************************************************************************
 * @brief Check if transition is currently in progress for a group and retrieve the remaining
 * transition time if needed
 *
 * @param[in] grp_lid       Group local index
 * @param[in] p_rem_time    Pointer to the variable in which remaining time will be written
 * if a transition is in progress
 *
 * @return true if transition is in progress else false
 ****************************************************************************************
 */
uint8_t mm_bind_get_trans_info(m_lid_t grp_lid, uint8_t *p_trans_type, uint8_t *p_rem_time);

/**
 ****************************************************************************************
 * @brief
 *
 * @param[in] p_mdl_env     
 * @param[in] src           Source addr
 * @param[in] tid           TID value
 *
 * @return
 ****************************************************************************************
 */
bool mm_replay_is_retx(mm_replay_env_t *p_mdl_env, uint16_t src, uint8_t tid);

/**
 ****************************************************************************************
 * @brief Allocate a buffer for transmission of a message
 *
 * @param[in] data_len   Requested data length
 ****************************************************************************************
 */
__INLINE__ mesh_buf_t* mm_route_buf_alloc(uint16_t data_len)
{
    // Allocate buffer and return the status
    return (mesh_buf_alloc(2, data_len, 0));
}

/**
 ****************************************************************************************
 * @brief Allocate a buffer for transmission of a status message sent as a direct answer.
 *
 * @param[in] data_len      Requested data length
 * @param[in] p_route_env   Pointer at which address of allocated buffer must be written
 ****************************************************************************************
 */
mesh_buf_t* mm_route_buf_alloc_status(uint16_t data_len, mm_route_env_t *p_route_env);

/**
 ****************************************************************************************
 * @brief Provide a buffer to the Access Layer of the Mesh Profile for transmission
 *
 * @param[in] p_buf     Pointer to buffer containing the message to send
 ****************************************************************************************
 */
void mm_route_send(mesh_buf_t *p_buf);


/****************************************************************************************
 * APIs - Send Messages to application
 ****************************************************************************************/

/**
 ****************************************************************************************
 * @brief Inform upper application about registered model
 *        Send a MESH_MDL_IND message with MM_REGISTER_IND indication code.
 *
 * @param[in] model_id      Model Identifier
 * @param[in] elmt_idx      Element Index
 * @param[in] mdl_lid       Allocated model local index
 ****************************************************************************************
 */
//void mm_register_ind_send(uint32_t model_id, uint8_t elmt_idx, m_lid_t mdl_lid);
#define mm_register_ind_send(model_id, elmt_idx, mdl_lid)

/**
 ****************************************************************************************
 * @brief Inform application about a local model state update
 *        Send a MESH_MDL_IND message with MM_SRV_STATE_UPD_IND indication code.
 *
 * @param[in] state_id      State identifier (@see mm_state_idx)
 * @param[in] elmt_idx      Element Index
 * @param[in] state         New state value or targeted state value depending on
 * transition time
 * @param[in] trans_time_ms Transition time in milliseconds
 ****************************************************************************************
 */
void mm_srv_state_upd_ind_send(uint16_t state_id, uint8_t elmt_idx, uint32_t state,
                                   uint32_t trans_time_ms);

/**
 ****************************************************************************************
 * @brief Inform upper application about set global part of Generic Location state value
 *        Send a MESH_MDL_IND message with MM_SRV_LOCG_UPD_IND indication code.
 *
 * @param[in] elmt_idx      Element Index
 * @param[in] latitude      Global Latitude
 * @param[in] longitude     Global Longitude
 * @param[in] altitude      Global Altitude
 ****************************************************************************************
 */
void mm_srv_locg_upd_ind_send(uint8_t elmt_idx, int32_t latitude, int32_t longitude,
                                    int16_t altitude);

/**
 ****************************************************************************************
 * @brief Inform upper application about set local part of Generic Location state value
 *        Send a MESH_MDL_IND message with MM_SRV_LOCL_UPD_IND indication code.
 *
 * @param[in] elmt_idx      Element Index
 * @param[in] north         Local North
 * @param[in] east          Local East
 * @param[in] altitude      Local Altitude
 * @param[in] floor         Floor Number
 * @param[in] uncertainty   Uncertainty
 ****************************************************************************************
 */
void mm_srv_locl_upd_ind_send(uint8_t elmt_idx, int16_t north, int16_t east, int16_t altitude,
                                   uint8_t floor, uint16_t uncertainty);

/**
 ****************************************************************************************
 * @brief Send a MESH_MDL_REQ_IND message to upper application in order to retrieve 
 *        a state value not stored locally
 *
 * @param[in] req_code      Request indication code (@see mm_req_code)
 * @param[in] elmt_dix      Element Index
 ****************************************************************************************
 */
void mm_srv_state_req_ind_send(uint16_t req_code, uint8_t elmt_idx);

/**
 ****************************************************************************************
 * @brief Send a MESH_MDL_REQ_IND message with MM_SRV_PROP_GET_REQ_IND to Inform application
 *        about received get request for a Generic User/Admin/Manufacturer Property.
 *        A confirmation is expected from the application.
 *
 * @param[in] elmt_idx      Index of element for which get request has been received
 * @param[in] prop_type     Property type (@see enum mm_prop_type)
 * @param[in] prop_id       Property ID
 ****************************************************************************************
 */
void mm_srv_prop_get_send(uint8_t elmt_idx, uint8_t prop_type, uint16_t prop_id);

/**
 ****************************************************************************************
 * @brief Send a MESH_MDL_REQ_IND message with MM_SRV_PROP_SET_REQ_IND to Inform application 
 *        about received set request for a Generic User/Admin/Manufacturer Property.
 *        A confirmation is expected from the application.
 *
 * @param[in] elmt_idx      Index of element for which set request has been received
 * @param[in] prop_type     Property type (@see enum mm_prop_type)
 * @param[in] prop_id       Property ID
 * @param[in] length        Property value length
 * @param[in] p_val         Pointer to the received property value
 ****************************************************************************************
 */
void mm_srv_prop_set_send(uint8_t elmt_idx, uint8_t prop_type, uint16_t prop_id,
                              uint16_t length, uint8_t *p_val);

/**
 ****************************************************************************************
 * @brief Inform application about received state value from an element
 *        Send a MESH_MDL_IND message with MM_CLI_STATE_IND indication code.
 *
 * @param[in] src           Address of element for which state value has been received
 * @param[in] state_id      State Identifier
 * @param[in] state_1       State value 1
 * @param[in] state_2       State value 2
 * @param[in] rem_time_ms   Remaining time in milliseconds
 ****************************************************************************************
 */
void mm_cli_state_ind_send(uint16_t src, uint16_t state_id, uint32_t state_1,
                               uint32_t state_2, uint32_t rem_time_ms);

/**
 ****************************************************************************************
 * @brief Inform application about received Generic Battery state value from an element
 *        Send a MESH_MDL_IND message with MM_CLI_BAT_IND indication code.
 *
 * @param[in] src           Address of node's element that has reported its Generic Battery state
 * @param[in] bat_lvl       Battery level value
 * @param[in] time_dischrg  Time to discharge
 * @param[in] time_chrg     Time to charge
 * @param[in] flags         Flags
 ****************************************************************************************
 */
void mm_cli_bat_ind_send(uint16_t src, uint8_t bat_lvl, uint32_t time_dischrg,
                             uint32_t time_chrg, uint8_t flags);

/**
 ****************************************************************************************
 * @brief Inform application about received Generic Location Global state value from an element
 *        Send a MESH_MDL_IND message with a MM_CLI_LOCG_IND indication code.
 *
 * @param[in] src           Address of node's element that has reported its Generic Location state
 * @param[in] latitude      Global Latitude
 * @param[in] longitude     Global Longitude
 * @param[in] altitude      Global Altitude
 ****************************************************************************************
 */
void mm_cli_locg_ind_send(uint16_t src, int32_t latitude, int32_t longitude, int16_t altitude);

/**
 ****************************************************************************************
 * @brief Inform application about received Generic Location Local state value from an element
 *        Send a MESH_MDL_IND message with a MM_CLI_LOCL_IND indication code.
 *
 * @param[in] src           Address of node's element that has reported its Generic Location state
 * @param[in] north         Local North
 * @param[in] East          Local East
 * @param[in] altitude      Local Altitude
 * @param[in] floor         Floor
 * @param[in] uncertainty   Uncertainty
 ****************************************************************************************
 */
void mm_cli_locl_ind_send(uint16_t src, int16_t north, int16_t east, int16_t altitude,
                               uint8_t floor, uint16_t uncertainty);

/**
 ****************************************************************************************
 * @brief Inform application about reception of a Generic User/Admin/Manufacturer Property value.
 *        Send a MESH_MDL_IND message with a MM_CLI_PROP_IND indication code.
 *
 * @param[in] src           Address of element for which property value has been received
 * @param[in] prop_id       Property ID
 * @param[in] user_access   User access
 * @param[in] length        Property value length
 * @param[in] p_val         Pointer to the property value
 ****************************************************************************************
 */
void mm_cli_prop_ind_send(uint16_t src, uint16_t prop_id, uint8_t user_access, uint16_t length,
                              uint8_t *p_val);

/**
 ****************************************************************************************
 * @brief Inform application about reception of list of Generic User/Admin/Manufacturer/Client
 *        Properties supported by an element. 
 *        Send a MESH_MDL_IND message with a MM_CLI_PROP_IND indication code.
 *
 * @param[in] src           Address of element for which list of properties has been received
 * @param[in] prop_type     Property type (@see enum mm_prop_type)
 * @param[in] nb_prop       Number of Properties
 * @param[in] p_prop_ids    Pointer to received list of Property IDs
 ****************************************************************************************
 */
void mm_cli_prop_list_ind_send(uint16_t src, uint8_t prop_type, uint16_t nb_prop,
                                   uint16_t *p_prop_ids);


#endif // _MM_ITF_H_
