/**
 ****************************************************************************************
 *
 * @file mesh.h
 *
 * @brief Header file for Mesh Application Interface
 *
 ****************************************************************************************
 */

#ifndef _MESH_H_
#define _MESH_H_

/**
 ****************************************************************************************
 * @defgroup MESH_API Mesh Application Interface
 * @ingroup MESH
 * @brief  Mesh Application Interface
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "mesh_cfg.h"
#include "mesh_err.h"
#include "mesh_msg.h"
#include "mesh_def.h"
#include "mm_def.h"


/*
 * DEFINES
 ****************************************************************************************
 */

/// Size of environment part in buffer provided by the Buffer Manager
#define MESH_BUF_ENV_SIZE               (32)
/// Block ID value for a dynamically allocated buffer
#define MESH_BUF_DYN_ALLOC              (0xFF)


/*
 * MACROS
 ****************************************************************************************
 */

/// Macro to get pointer to data part of a provided buffer
#define MESH_BUF_DATA(p_buf)            (&(p_buf)->buf[0] + (p_buf)->head_len)

/// Macro to get a structure from one of its structure field
#define MESH_TMR2ENV(ptr, env_type, tmr_member)     \
    ((env_type *)((uint8_t *)(ptr) - offsetof(env_type, tmr_member)))

/// Macro to get a structure from one of its structure field
#define MESH_DJOB2ENV(ptr, env_type, djob_member)   \
    ((env_type *)((uint8_t *)(ptr) - offsetof(env_type, djob_member)))


/*
 * TYPES FOR MESH STACK
 ****************************************************************************************
 */

/// Pointer to callback function called when djob execute
typedef void (*mesh_djob_cb)(void* p_djob);

/// Pointer to callback function called when timer expires
typedef void (*mesh_timer_cb)(void *p_timer);

/// Job delayed element structure
typedef struct mesh_djob
{
    /// List element header
    list_hdr_t      hdr;
    /// Callback to execute in background context
    mesh_djob_cb    cb;
} mesh_djob_t;

/// Structure defining the properties of a timer
typedef struct
{
    /// List element for chaining
    list_hdr_t      hdr;
    /// Function to be called upon timer expiration
    mesh_timer_cb   cb;
    /// Expiration time in milliseconds
    uint32_t        time_ms;
    /// Number of wraps
    uint16_t        nb_wrap;
    /// Not used by timer module but by client, this variable is used to save timer period information.
    /// There is no unity to let client use it with any kind of timing step.
    /// It is recommended to use it only to keep period an not other information
    uint16_t        period;
} mesh_timer_t;

/// Buffer information structure
typedef struct mesh_buf
{
    /// List header for chaining
    list_hdr_t hdr;
    /// Length of the head part
    uint16_t head_len;
    /// Length of the tail part
    uint16_t tail_len;
    /// Length of the data part
    uint16_t data_len;
    /// Total size of the buffer array
    uint16_t buf_len;
    /// Index of block the buffer belongs to
    /// Set to @see MESH_BUF_DYN_ALLOC if dynamically allocated
    uint8_t block_id;
    /// Index of the buffer in the block if buffer belongs to a block
    uint8_t buf_id;
    /// Acquire counter
    uint8_t acq_cnt;
    /// Dummy Tag that can be used to inform a layer about type of buffer.
    /// Value should not be kept along several layers
    uint8_t dummy_tag;
    /// Environment variable that can be used for multiple purposes
    uint8_t env[MESH_BUF_ENV_SIZE];
    /// Data buffer containing the PDU
    /// Length is head_len + tail_len + data_len
    uint8_t buf[];
} mesh_buf_t;


/*
 * TYPES FOR MESH SERVICE
 ****************************************************************************************
 */

typedef struct ms_prov_param
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
} ms_prov_param_t;

/// Low Power Node feature Configuration Structure
typedef struct ms_lpn_conf
{
    /// Initial value of PollTimeout timer (in multiple of 100ms)
    uint32_t    poll_timeout;
    /// Poll interval (in milliseconds)
    uint32_t    poll_intv_ms;
    /// Unicast address of the primary element of the previous friend
    uint16_t    prev_addr;
    /// Receive delay
    uint8_t     rx_delay;
    /// RSSI factor
    uint8_t     rssi_factor;
    /// Receive window factor
    uint8_t     rx_window_factor;
    /// Minimum queue size (log value)
    uint8_t     min_queue_size_log;
} ms_lpn_conf_t;

/// Structure containing information about a friendship established as Low Power Node
typedef struct ms_lpn_info
{
    /// Poll interval in milliseconds
    uint32_t poll_intv_ms;
    /// LPN Counter
    uint16_t lpn_cnt;
    /// Address of Friend node
    uint16_t frd_addr;
    /// Friend Counter
    uint16_t frd_cnt;
    /// Receive delay in milliseconds
    uint8_t  rx_delay_ms;
    /// Receive window in milliseconds
    uint8_t  rx_window_ms;
    /// Friend subscription list size
    uint8_t  subs_list_size;
    /// Friend Sequence Number
    uint8_t  fsn;
} ms_lpn_info_t;


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/****************************************************************************************
 * APIs - Mesh Stack
 ****************************************************************************************/

/**
 ****************************************************************************************
 * @brief Init Mesh Stack (Create task and Init environment).
 *
 * @param[in] feats         Feature supported (@see enum mesh_feat), 0 mean default value
 ****************************************************************************************
 */
void mesh_init(uint32_t feats);

/**
 ****************************************************************************************
 * @brief Start Mesh Profile, enable bearer layer.
 *
 * @return Status (@see enum mesh_err)
 ****************************************************************************************
 */
uint8_t mesh_enable(void);

/**
 ****************************************************************************************
 * @brief Get current time value, duration elapsed since device up time in milliseconds.
 *
 * @param[out] p_time_ms    Pointer to current time (ms part)
 * @param[out] p_nb_wrap    Pointer to current time (wrap part)
 ****************************************************************************************
 */
void mesh_clock_get(uint32_t *p_clock_ms, uint16_t *p_nb_wrap);

/**
 ****************************************************************************************
 * @brief Set mesh clock, duration elapsed since device up time in milliseconds.
 *
 * @param[in] clock_ms      Current clock  (ms part)
 * @param[in] nb_wrap       Number of wrap (wrap part)
 ****************************************************************************************
 */
void mesh_clock_set(uint32_t clock_ms, uint16_t nb_wrap);

/**
 ****************************************************************************************
 * @brief Allocate a buffer and specify initial length of head, data and tail parts.
 *
 * @param[in] head_len      Initial Head Length.
 * @param[in] data_len      Initial Data Length.
 * @param[in] tail_len      Initial Tail Length.
 *
 * @return Pointer to buffer if it can be allocated, NULL if no more buffers are available.
 ****************************************************************************************
 */
mesh_buf_t* mesh_buf_alloc(uint16_t head_len, uint16_t data_len, uint16_t tail_len);

/**
 ****************************************************************************************
 * @brief Request to release previously acquired buffer. The acquire counter for this buffer
 * is decremented. If the acquire counter value becomes zero, the buffer is freed as no more
 * entity is using the buffer anymore.
 *
 * @param[in] p_buf         Pointer to acquired buffer.
 *
 * @return MESH_ERR_NO_ERROR if buffer has been released.
 *         MESH_ERR_COMMAND_DISALLOWED if buffer was free.
 ****************************************************************************************
 */
uint8_t mesh_buf_release(mesh_buf_t *p_buf);

/**
 ****************************************************************************************
 * @brief Copy content of a buffer to another buffer.
 *
 * @param[in] dst           Pointer to the buffer that will contain content of input buffer.
 * @param[in] src           Pointer to the buffer to copy.
 * @param[in] length        Length of data to copy
 * @param[in] copy_env      Indicate if environment has to be copied.
 ****************************************************************************************
 */
void mesh_buf_copy(mesh_buf_t *dst, mesh_buf_t *src, uint16_t length, bool copy_env);

/// Copy data of a buffer to another buffer.
__INLINE__ void mesh_buf_copy_data(mesh_buf_t *dst, mesh_buf_t *src, uint16_t length)
{
    mesh_buf_copy(dst, src, length, false);
}

/// Copy env[] of a buffer to another buffer
__INLINE__ void mesh_buf_copy_env(mesh_buf_t *dst, mesh_buf_t *src)
{
    mesh_buf_copy(dst, src, 0, true);
}

/**
 ****************************************************************************************
 * @brief Register to execute a job delayed in background
 *
 * @param[in] p_djob        Pointer to the delayed job structure
 ****************************************************************************************
 */
void mesh_djob_reg(mesh_djob_t* p_djob);

/**
 ****************************************************************************************
 * @brief Program a timer to be scheduled in the future.
 *
 * @param[in] p_timer       Pointer to the timer structure.
 * @param[in] delay         Duration before expiration of the timer (in milliseconds)
 ****************************************************************************************
 */
void mesh_timer_set(mesh_timer_t *p_timer, uint32_t delay_ms);

/**
 ****************************************************************************************
 * @brief Clear a programmed timer.
 * This function searches for the timer passed as parameter in the list of programmed
 * timers and extract it from the list.
 *
 * @param[in] p_timer       Pointer to the timer to be cleared
 ****************************************************************************************
 */
void mesh_timer_clear(mesh_timer_t *p_timer);


/****************************************************************************************
 * APIs - Mesh Service
 ****************************************************************************************/

/**
 ****************************************************************************************
 * @brief Set Mesh profile SSID(service set identifier)
 *
 * @param[in] cid           16-bit company identifier assigned by the Bluetooth SIG
 * @param[in] pid           16-bit vendor-assigned product identifier
 * @param[in] vid           16-bit vendor-assigned product version identifier
 * @param[in] loc           Localization descriptor
 ****************************************************************************************
 */
void ms_set_ssid(uint16_t cid, uint16_t pid, uint16_t vid, uint16_t loc);

/**
 ****************************************************************************************
 * @brief Set Mesh default TTL value
 *
 * @param[in] dflt_ttl      Default TTL value (ttl <= MESH_TTL_MAX && ttl != 1 )
 *
 * @return Error status
 ****************************************************************************************
 */
uint8_t ms_set_dflt_ttl(uint8_t dflt_ttl);

/**
 ****************************************************************************************
 * @brief Provide provisioning parameters to the provisioning module
 *
 * @param[in] p_param       Provisioning parameters
 ****************************************************************************************
 */
void ms_prov_param_rsp(ms_prov_param_t *p_param);

/**
 ****************************************************************************************
 * @brief Provide authentication data to the provisioning module
 *
 * @param[in] accept        True, Accept pairing request, False reject
 * @param[in] auth_size     Authentication data size (<= requested size else pairing rejected)
 * @param[in] p_auth_data   Authentication data (LSB for a number or array of bytes)
 ****************************************************************************************
 */
void ms_prov_auth_rsp(bool accept, uint8_t auth_size, const uint8_t* p_auth_data);

/**
 ****************************************************************************************
 * @brief Provide a page of Composition Data to the provisioning module
 *
 * @param[in] page          Page number
 * @param[in] length        Length of composition data page
 * @param[in] p_data        Pointer to data
 ****************************************************************************************
 */
void ms_compo_data_rsp(uint8_t page, uint8_t length, uint8_t *p_data);

/**
 ****************************************************************************************
 * @brief Provide Registered Fault state for primary element to the hlths module.
 *
 * @param[in] comp_id       Company ID
 * @param[in] test_id       Test ID
 * @param[in] length        Length of fault array
 * @param[in] p_fault       Pointer to fault array content
 ****************************************************************************************
 */
void ms_fndh_fault_rsp(bool accept, uint16_t comp_id, uint8_t test_id, uint8_t length, uint8_t *p_fault);

/**
 ****************************************************************************************
 * @brief Control if Proxy service should start / stop advertising it's capabilities
 *
 * @param[in] enable        True to enable advertising for 60s, 
 *                          False to stop (@see enum mesh_proxy_adv_ctl)
 *
 * @return Execution status code
 ****************************************************************************************
 */
uint8_t ms_proxy_ctrl(uint8_t enable);


/****************************************************************************************
 * APIs - Mesh Models
 ****************************************************************************************/

/**
 ****************************************************************************************
 * @brief Set state value
 *
 * @param[in] p_env         Pointer to the environment allocated for the model
 * @param[in] state_id      State identifier (@see enum mm_state_id)
 * @param[in] state         State value
 *
 * @return An error status
 ****************************************************************************************
 */
uint8_t mm_state_srv_set(m_lid_t mdl_lid, uint16_t state_id, uint32_t state);

/**
 ****************************************************************************************
 * @brief Get value of a state
 *
 * @param[in] mdl_lid       Local index for the client model used to get the needed state value
 * @param[in] dst           Element address to which the get message must be sent
 * @param[in] get_info      Get information
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
uint8_t mm_state_cli_get(m_lid_t mdl_lid, m_lid_t app_key_lid, uint16_t dst, uint16_t get_info);

/**
 ****************************************************************************************
 * @brief Set value of a state
 *
 * @param[in] mdl_lid       Local index for the client model used to set the needed state value
 * @param[in] dst           Element address to which the set message must be sent
 * @param[in] state_1       State value 1
 * @param[in] state_2       State value 2
 * @param[in] set_info      Set information (@see enum mm_set_info)
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
uint8_t mm_state_cli_set(m_lid_t mdl_lid, m_lid_t app_key_lid, uint16_t dst, uint32_t state_1,
                        uint32_t state_2, uint16_t set_info);

/**
 ****************************************************************************************
 * @brief Set value of a state using a transition
 *
 * @param[in] mdl_lid        Local index for the client model used to set the needed state value
 * @param[in] dst            Element address to which the set message must be sent
 * @param[in] state_1        State value 1
 * @param[in] state_2        State value 2
 * @param[in] trans_time_ms  Transition Time in milliseconds
 * @param[in] delay_ms       Delay in milliseconds
 * @param[in] trans_info     Transition Information (@see mm_trans_info)
 *
 * @return An error status (@see enum mesh_err)
 ****************************************************************************************
 */
uint8_t mm_state_cli_trans(m_lid_t mdl_lid, m_lid_t app_key_lid, uint16_t dst, uint32_t state_1,
                               uint32_t state_2, uint32_t trans_time_ms, uint16_t delay_ms,
                               uint16_t trans_info);

/// @} MESH_API

#endif /* _MESH_H_ */
