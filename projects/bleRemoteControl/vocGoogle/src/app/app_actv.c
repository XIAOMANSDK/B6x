/**
 ****************************************************************************************
 *
 * @file app_actv.c
 *
 * @brief Application Activity(Advertising, Scanning and Initiating) - Example
 *
 * < If want to modify it, recommend to copy the file to 'user porject'/src >
 ****************************************************************************************
 */

#include "app.h"
#include "gapm_api.h"
#include "app_user.h"
#include "ble_priv_data.h"

#if (DBG_ACTV)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<ADV>" format "\r\n", ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif


/*
 * DEFINES
 ****************************************************************************************
 */

/// Index & State of activities - User Customize
struct actv_env_tag
{
    #if (BLE_EN_ADV)
    /// Advertising index and state
    uint8_t advidx;
    uint8_t advsta;

    ble_adv_type_t advtype;
    #endif //(BLE_EN_ADV)
};

/// Activities environment
struct actv_env_tag actv_env = {0};


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @section Advertising Activity - Example for User Customize
 *          enable via pre-define @see BLE_EN_ADV
 ****************************************************************************************
 */
#if (BLE_EN_ADV)


/*
 * DEFINITIONS
 ****************************************************************************************
 */

#include "adv_data.h"

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

// 广播超时处理
static void ble_adv_timeout_proc(uint8_t status)
{
    if (status == GAP_ERR_TIMEOUT)
    {
        DEBUG("adv %d timeout\n", actv_env.advtype);
        if (actv_env.advtype == ADV_POWER)
        {
            ble_set_adv_type(ADV_DIRECT);
            app_adv_action(ACTV_RELOAD);
        }
        else
        {
            keys_sleep();
        }
    }
}

static void app_adv_create(void)
{
    if (actv_env.advtype >= ADV_TYPE_MAX)
        return;

    struct gapm_adv_create_param adv_param;

    // Advertising type (@see enum gapm_adv_type)
    adv_param.type                  = GAPM_ADV_TYPE_LEGACY;
    // Discovery mode (@see enum gapm_adv_disc_mode)
    adv_param.disc_mode             = ble_adv_params[actv_env.advtype].disc_mode;
    // Advertising properties (@see enum gapm_adv_prop)
    adv_param.prop                  = ble_adv_params[actv_env.advtype].prop;
    // Filtering policy (@see enum gapm_adv_filter_policy)
    adv_param.filter_pol            = GAPM_ADV_ALLOW_SCAN_ANY_CON_ANY;
    // Config primary advertising (@see gapm_adv_prim_cfg)
    adv_param.prim_cfg.phy          = GAP_PHY_LE_1MBPS;
    adv_param.prim_cfg.chnl_map     = 0x07; /*!< Advertising channel map - 37, 38, 39 */
    adv_param.prim_cfg.adv_intv_min = ble_adv_params[actv_env.advtype].interval;
    adv_param.prim_cfg.adv_intv_max = ble_adv_params[actv_env.advtype].interval;

    const uint8_t *peer_addr = ble_read_peer_addr();
    if (actv_env.advtype == ADV_POWER)
    {
        app_adv_data_power[28] = peer_addr[0];
        app_adv_data_power[29] = peer_addr[1];
        app_adv_data_power[30] = peer_addr[2];
        DEBUG("adv_data_power %X %X %X\n", app_adv_data_power[28], app_adv_data_power[29], app_adv_data_power[30]);
    }
    else if (actv_env.advtype == ADV_DIRECT)
    {
        memcpy(&adv_param.peer_addr.addr.addr[0], peer_addr, GAP_BD_ADDR_LEN);
        adv_param.peer_addr.addr_type = 0; /*!< 对端地址类型 public */
        // ke_timer_set(APP_TIMER_KEY_ADV_DIR, TASK_APP, 1300);  // 1.3s, 超时重新配对广播
    }

    DEBUG("[adv] type:%d,disc:%d,prop:%d\n", actv_env.advtype, adv_param.disc_mode, adv_param.prop);

    gapm_create_advertising(GAPM_STATIC_ADDR, &adv_param);
}

static void app_adv_set_adv_data(void)
{
    gapm_set_adv_data(actv_env.advidx, GAPM_SET_ADV_DATA, 
                    ble_adv_params[actv_env.advtype].adv_data_size, ble_adv_params[actv_env.advtype].adv_data);
}

static void app_adv_set_scan_rsp(void)
{
    uint8_t length;
    uint8_t rsp_data[DEV_NAME_MAX_LEN+2];

    if (actv_env.advtype == ADV_DIRECT)
    {
        gapm_set_adv_data(actv_env.advidx, GAPM_SET_SCAN_RSP_DATA, 0, NULL);
    }
    else
    {
        // Set device name
        length = app_name_get(DEV_NAME_MAX_LEN, &rsp_data[2]);
        rsp_data[0] = length + 1;
        rsp_data[1] = GAP_AD_TYPE_COMPLETE_NAME; // 0x09

        gapm_set_adv_data(actv_env.advidx, GAPM_SET_SCAN_RSP_DATA, length + 2, rsp_data);
    }
}

/**
 ****************************************************************************************
 * @brief Action/Command of Advertising
 *
 * @param[in] actv_op  Operation of activity
 ****************************************************************************************
 */
void app_adv_action(uint8_t actv_op)
{
    switch (actv_op)
    {
        case ACTV_CREATE:
        {
            if (actv_env.advsta == ACTV_STATE_OFF)
            {
                //DEBUG("Creating");
                app_adv_create();
                actv_env.advsta = ACTV_STATE_CREATE;
            }
        } break;

        case ACTV_START:
        {
            if (actv_env.advsta == ACTV_STATE_READY)
            {
                DEBUG("Starting");
                gapm_start_advertising(actv_env.advidx, ble_adv_params[actv_env.advtype].timeout);
                actv_env.advsta = ACTV_STATE_START;
            }
        } break;
        
        case ACTV_STOP:
        {
            if (actv_env.advsta == ACTV_STATE_START)
            {
                DEBUG("Stopping");
                gapm_stop_activity(actv_env.advidx);
                actv_env.advsta = ACTV_STATE_STOP;
            }
        } break;
        
        case ACTV_DELETE:
        {
            if ((actv_env.advsta != ACTV_STATE_OFF) && (actv_env.advsta != ACTV_STATE_START))
            {
                DEBUG("Deleting");
                gapm_delete_activity(actv_env.advidx);
                actv_env.advsta = ACTV_STATE_OFF;
            }
        } break;
        
        case ACTV_RELOAD:
        {
            if (actv_env.advsta == ACTV_STATE_START)
            {
                DEBUG("Stopping");
                gapm_stop_activity(actv_env.advidx);
                actv_env.advsta = ACTV_STATE_STOP;
            }
            
            if ((actv_env.advsta != ACTV_STATE_OFF) && (actv_env.advsta != ACTV_STATE_START))
            {
                DEBUG("Deleting");
                gapm_delete_activity(actv_env.advidx);
                actv_env.advsta = ACTV_STATE_OFF;
            }
            
            app_adv_create();
            actv_env.advsta = ACTV_STATE_CREATE;
        } break;
        
        default:
            break;
    }
}

/**
 ****************************************************************************************
 * @brief Event Procedure of Advertising 
 *
 * @param[in] gapm_op  Operation of gapm
 * @param[in] status   Status of event
 ****************************************************************************************
 */
void app_adv_event(uint8_t gapm_op, uint8_t status)
{
    DEBUG("Evt(op:0x%X,sta:0x%X)", gapm_op, status);
    
    switch (gapm_op)
    {
        case (GAPM_CREATE_ADV_ACTIVITY):
        {
            app_adv_set_adv_data();
        } break;
        
        case (GAPM_SET_ADV_DATA):
        {
            app_adv_set_scan_rsp();
        } break;
        
        case (GAPM_SET_SCAN_RSP_DATA):
        {
            actv_env.advsta = ACTV_STATE_READY;
            
            app_adv_action(ACTV_START);            
            app_state_set(APP_READY);
        } break;
        
        case (GAPM_STOP_ACTIVITY):
        {
            if ((actv_env.advsta == ACTV_STATE_START) || (actv_env.advsta == ACTV_STATE_STOP))
            {
                actv_env.advsta = ACTV_STATE_READY;
            }
            ble_adv_timeout_proc(status);
        } break;
        
        default:
            break;
    }
}

#endif //(BLE_EN_ADV)

/**
 ****************************************************************************************
 * @brief Create activities when Initialization complete.
 ****************************************************************************************
 */
void app_actv_create(void)
{
    memset(&actv_env, 0, sizeof(actv_env));
    #if (BLE_EN_ADV)
    app_adv_action(ACTV_CREATE);
    #endif //(BLE_EN_ADV)
}

/**
 ****************************************************************************************
 * @brief Handles activity command complete event.
 *
 * @param[in] gapm_op  Operation of gapm
 * @param[in] status   Status of event
 ****************************************************************************************
 */
void app_actv_cmp_evt(uint8_t operation, uint8_t status)
{
    switch (operation)
    {
        #if (BLE_EN_ADV)
        case (GAPM_CREATE_ADV_ACTIVITY):
        case (GAPM_SET_ADV_DATA):
        case (GAPM_SET_SCAN_RSP_DATA):     
        {
            app_adv_event(operation, status);
        } break;  
        #endif //(BLE_EN_ADV)
        default:
            break;
    }
}

/**
 ****************************************************************************************
 * @brief Handles activity created. (@see GAPM_ACTIVITY_CREATED_IND)
 *
 * @param[in] actv_type  Type of activities(@see enum gapm_actv_type)
 * @param[in] actv_idx   Index of activities created
 ****************************************************************************************
 */
void app_actv_created_ind(uint8_t actv_type, uint8_t actv_idx)
{
    switch (actv_type)
    {
        #if (BLE_EN_ADV)
        case GAPM_ACTV_TYPE_ADV:
        {
            actv_env.advidx = actv_idx;
            if (actv_env.advidx == 1)
            {
                DEBUG("\nadv created repeatedly!!!\n");
            }
        } break;
        #endif //(BLE_EN_ADV)

        default:
            break;
    }
}

/**
 ****************************************************************************************
 * @brief Handles activity stopped. (@see GAPM_ACTIVITY_STOPPED_IND)
 *
 * @param[in] actv_type  Type of activity(@see enum gapm_actv_type)
 * @param[in] reason     Reason of stopped
 ****************************************************************************************
 */
void app_actv_stopped_ind(uint8_t actv_type, uint8_t actv_idx, uint8_t reason)
{
    switch (actv_type)
    {
        #if (BLE_EN_ADV)
        case GAPM_ACTV_TYPE_ADV:
        {
            // Advertising Stopped by slave connection or duration timeout
            app_adv_event(GAPM_STOP_ACTIVITY, reason);

            // Duration timeout, go IDLE - update from v1.3
            if ((reason == GAP_ERR_TIMEOUT) && (app_state_get() == APP_READY))
            {
                app_state_set(APP_IDLE);
            }
        } break;
        #endif //(BLE_EN_ADV)

        default:
            break;
    }
}

void ble_set_adv_type(ble_adv_type_t advtype)
{
    // /********************************************************/
    // // Add. 2024.05.22 --- wq.
    // if ((advtype == ADV_DIRECT) && (!peer_addr_valid()))
    // {
    //     advtype = ADV_REPAIR;
    // }
    // /********************************************************/

    DEBUG("ble_set_adv_type %d\n", advtype);
    actv_env.advtype = advtype;
}

ble_adv_type_t ble_get_adv_type(void)
{
    return actv_env.advtype;
}

