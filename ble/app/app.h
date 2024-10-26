/**
 ****************************************************************************************
 *
 * @file app.h
 *
 * @brief Header file - Application Defines
 *
 ****************************************************************************************
 */

#ifndef _APP_H_
#define _APP_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include <stdint.h>
#include "string.h"
#include "blelib.h"
#include "ke_api.h"
#include "app_actv.h"


/*
 * DEFINES
 ****************************************************************************************
 */

/// Number of Roles, Local Device act as Slave or Master
#if !defined(BLE_NB_SLAVE)
    #define BLE_NB_SLAVE          (1)
#endif

#if !defined(BLE_NB_MASTER)
    #define BLE_NB_MASTER         (0)
#endif

#if (BLE_NB_SLAVE + BLE_NB_MASTER > BLE_CONNECTION_MAX)
    #error "Number of roles must not exceed BLE_CONNECTION_MAX"
#endif

/// Single or Multiple Connections
#if (BLE_NB_SLAVE + BLE_NB_MASTER > 1)
    #define BLE_MULTI_CONN        (1)
#else
    #define BLE_MULTI_CONN        (0)
#endif

/// Enable of Activities(adv/scan/init) @see app_actv.c
#if !defined(BLE_EN_ADV)
    #define BLE_EN_ADV            (BLE_NB_SLAVE)
#endif

#if !defined(BLE_EN_SCAN)
    #define BLE_EN_SCAN           (BLE_NB_MASTER)
#endif

#if !defined(BLE_EN_INIT)
    #define BLE_EN_INIT           (BLE_NB_MASTER)
#endif

#if !defined(APP_ACTV_EN)
    #define APP_ACTV_EN           (BLE_EN_ADV || BLE_EN_SCAN || BLE_EN_INIT)
#endif

/// Enable Pairing and Bond, used debugLTK @see app_gapc.c
#if !defined(BLE_EN_SMP)
    #define BLE_EN_SMP            (1)
#endif

#if !defined(BLE_DBG_LTK)
    #define BLE_DBG_LTK           (0)
#endif

/// Maximal Transmission Unit
#if !defined(BLE_MTU)
    #define BLE_MTU               (124)
#endif

/// Maximal length of the Device Name value
#define DEV_NAME_MAX_LEN          (20)

/// Generic Access Service
#if !defined(BLE_DEV_ICON)
    #define BLE_DEV_ICON          (0x0000) // Unknow appearance
#endif

/// Period of RC32K Calibration(0 means only calib once when init)
#if !defined(RC32K_CALIB_PERIOD) && (CFG_SLEEP)
    #define RC32K_CALIB_PERIOD    (15000) // unit in 1ms
#endif

#if ((RC32K_CALIB_PERIOD > 0) && (RC32K_CALIB_PERIOD < 1000) && (CFG_SLEEP))
    #error "RC32K_CALIB_PERIOD Less than 1000ms"
#endif

/*
 * MACROS
 ****************************************************************************************
 */

/// Macro of Application Handler Functions
#define APP_SUBTASK_HANDLER(sub)   int app_##sub##_handler(msg_id_t msgid, const void *param, \
                                                       task_id_t dest_id, task_id_t src_id)

#define APP_MSG_HANDLER(msg)       static void app_##msg##_handler(msg_id_t msgid, struct msg const *param, \
                                                       task_id_t dest_id, task_id_t src_id)

#define APP_MSG_HANDLER_T(msg)     static void app_##msg##_handler(msg_id_t msgid, const void *param, \
                                                       task_id_t dest_id, task_id_t src_id)

#define APP_MSG_FUNCTION(msg)      app_##msg##_handler(msgid, param, dest_id, src_id)


/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// APP Task messages
enum app_msg_id
{
    APP_BASE_MSG = MSG_ID(APP, 0),

    /* Add more messages, such as TIMEOUT */
    APP_TIMER_RC32K_CORR,
    APP_TIMER_10MINUTES,
};

/// States of APP task
enum app_state_type
{
    // Init state
    APP_INIT,
    // Idle state
    APP_IDLE,
    // Ready State
    APP_READY,
    // Connected state
    APP_CONNECTED,
    APP_PAIRING,
    APP_BONDED,
    APP_ENCRYPTED,

    // Number of defined states.
    APP_STATE_MAX
};

/// Event of Ble Finite state machine
enum ble_event
{
    // Configure Events
    BLE_RESET,
    BLE_CONFIGURED,

    // Connection Events
    BLE_CONNECTED,
    BLE_DISCONNECTED,
    BLE_BONDED,
    BLE_ENCRYPTED,
};


/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Application environment structure
struct app_env_tag
{
    // Application State
    state_t state;

    // Curr Connection Index
    uint8_t curidx;

    #if (BLE_MULTI_CONN)
    // Bits of Connection Index(0:Not Connect, 1:Connected)
    uint8_t conbits;
    // Bits of Connection Roles(0:Master or None, 1:Slave)
    uint8_t conrole;
    #endif //(BLE_MULTI_CONN)
};


/*
 * VARIABLE DECLARATION
 ****************************************************************************************
 */

/// Application environment
extern struct app_env_tag app_env;

/// Ble local address (user customize)
extern const bd_addr_t ble_dev_addr;

/// GAP device configuration
extern const struct gapm_dev_config ble_dev_config;


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief API to Init Application, maybe User Override! (__weak func)
 *
 * @param[in] rsn   reset reason @see enum rst_src_bfs
 ****************************************************************************************
 */
void app_init(uint16_t rsn);

/**
 ****************************************************************************************
 * @brief API to Create Profiles, maybe User Override! (__weak func)
 *        Added in order and judged status in each profile-func.
 ****************************************************************************************
 */
void app_prf_create(void);

/**
 ****************************************************************************************
 * @brief API to Create Mesh Instance, maybe User Override! (__weak func)
 *        Config Mesh stack and Register models.
 ****************************************************************************************
 */
void app_mesh_create(void);

/**
 ****************************************************************************************
 * @brief Finite state machine for Device Configure, maybe User Override! (__weak func)
 *
 * @param[in] evt   configure event @see enum ble_event
 ****************************************************************************************
 */
void app_conf_fsm(uint8_t evt);

/**
 ****************************************************************************************
 * @brief Finite state machine for connection event, maybe User Override! (__weak func)
 *
 * @param[in] evt     connection event @see enum ble_event
 * @param[in] conidx  connection index
 * @param[in] param   param of connection event
 ****************************************************************************************
 */
void app_conn_fsm(uint8_t evt, uint8_t conidx, const void*param);

/**
 ****************************************************************************************
 * @brief API to Set State of Application, maybe User Override! (__weak func)
 *
 * @param[in] state    new state
 ****************************************************************************************
 */
void app_state_set(uint8_t state);

/**
 ****************************************************************************************
 * @brief Macro API to Get Application State
 *
 * @return state type - 1 octets
 ****************************************************************************************
 */
static __inline state_t app_state_get(void)
{
    return app_env.state;
}

/**
 ****************************************************************************************
 * @brief Macro API to Get Device Appearance
 *
 * @return icon type - 2 octets
 ****************************************************************************************
 */
static __inline uint16_t app_icon_get(void)
{
    return BLE_DEV_ICON;
}

/**
 ****************************************************************************************
 * @brief API to Get Device Name, maybe User Override! (__weak func)
 *
 * @param[in]     size   Length of name Buffer
 * @param[out] name   Pointer of name buffer
 *
 * @return Length of device name
 ****************************************************************************************
 */
uint8_t app_name_get(uint8_t size, uint8_t *name);

/**
 ****************************************************************************************
 * @brief API to Get Pairing Feature, maybe User Override! (__weak func)
 *
 * @param[out] feat   Pointer of pairing buffer to fill
 ****************************************************************************************
 */
void app_pairing_get(struct gapc_pairing *feat);

/**
 ****************************************************************************************
 * @brief API to Generate LTK for bonding, maybe User Override! (__weak func)
 *
 * @param[in]  conidx   connection index
 * @param[out] ltk      Pointer of LTK buffer to fill
 ****************************************************************************************
 */
void app_ltk_gen(uint8_t conidx, struct gapc_ltk *ltk);

/**
 ****************************************************************************************
 * @brief API to Save LTK when bonded, maybe User Override! (__weak func)
 *
 * @param[in] conidx   connection index
 * @param[in] ltk      Pointer of LTK data
 ****************************************************************************************
 */
void app_ltk_save(uint8_t conidx, const struct gapc_ltk *ltk);

/**
 ****************************************************************************************
 * @brief API to Find LTK when re-encryption, maybe User Override! (__weak func)
 *
 * @param[in] ediv     EDIV value for matching
 * @param[in] rand_nb  Rand Nb values for matching
 *
 * @return NULL for not matched, else return Pointer of LTK found.
 ****************************************************************************************
 */
const uint8_t *app_ltk_find(uint16_t ediv, const uint8_t *rand_nb);

/**
 ****************************************************************************************
 * @brief Dispatch TASK_APP message to sub-handler.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] task_idx  Index of the receiving task instance.
 *
 * @return Handler of the message or NULL.
 ****************************************************************************************
 */
void* app_task_dispatch(msg_id_t msgid, uint8_t task_idx);

#endif // _APP_H_
