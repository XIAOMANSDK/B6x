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
#include "atcmd.h"

#if (DBG_ACTV)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
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
    #endif //(BLE_EN_ADV)
    
    #if (BLE_EN_SCAN)
    /// Scanning index and state
    uint8_t scanidx;
    uint8_t scansta;
    #endif //(BLE_EN_SCAN)
    
    #if (BLE_EN_INIT)
    /// Initiating index and state
    uint8_t initidx;
    uint8_t initsta;
    #endif //(BLE_EN_INIT)
};

/// Activities environment
struct actv_env_tag actv_env;


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

/// Advertising duration - 0 mean Always ON (in multiple of 10ms)
#if !defined(APP_ADV_DURATION)
    #define APP_ADV_DURATION  (0)
#endif

/// Advertising channel map - 37, 38, 39
#if !defined(APP_ADV_CHMAP)
    #define APP_ADV_CHMAP     (0x07)
#endif

/// Advertising minimum interval - (n)*0.625ms
#if !defined(APP_ADV_INT_MIN)
    #define APP_ADV_INT_MIN   (64)
#endif

/// Advertising maximum interval - (n)*0.625ms
#if !defined(APP_ADV_INT_MAX)
    #define APP_ADV_INT_MAX   (160)
#endif

#if (APP_ADV_INT_MIN > APP_ADV_INT_MAX)
    #error "ADV_INT_MIN must not exceed ADV_INT_MAX"
#endif

/// Fast advertising interval
#define APP_ADV_FAST_INT      (32)

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

static void app_adv_create(void)
{
    struct gapm_adv_create_param adv_param;

    // Advertising type (@see enum gapm_adv_type)
    adv_param.type                  = GAPM_ADV_TYPE_LEGACY;
    // Discovery mode (@see enum gapm_adv_disc_mode)
    adv_param.disc_mode             = GAPM_ADV_MODE_GEN_DISC;
    // Advertising properties (@see enum gapm_adv_prop)
    adv_param.prop                  = GAPM_ADV_PROP_UNDIR_CONN_MASK;

    // Filtering policy (@see enum gapm_adv_filter_policy)
    adv_param.filter_pol            = GAPM_ADV_ALLOW_SCAN_ANY_CON_ANY;
    // Config primary advertising (@see gapm_adv_prim_cfg)
    adv_param.prim_cfg.phy          = GAP_PHY_LE_1MBPS;
    adv_param.prim_cfg.chnl_map     = APP_ADV_CHMAP;
    adv_param.prim_cfg.adv_intv_min = sys_config.adv_intv_time;//APP_ADV_INT_MIN;
    adv_param.prim_cfg.adv_intv_max = sys_config.adv_intv_time;//APP_ADV_INT_MAX;

    DEBUG("create(disc:%d,prop:%d)\r\n", adv_param.disc_mode, adv_param.prop);
    
    gapm_create_advertising(GAPM_STATIC_ADDR, &adv_param);
}

static void app_adv_set_adv_data(void)
{
#if (0)
    // Reserve 3Bytes for AD_TYPE_FLAGS
    uint8_t adv_data[GAP_ADV_DATA_LEN]; 
    uint8_t length = 11;
    
    // Set flags: 3B
    adv_data[0] = 0x02;
    adv_data[1] = GAP_AD_TYPE_FLAGS; // 0x01 0xFF;//
    adv_data[2] = 0x06;
    
    // Set list of UUIDs: 4B
    adv_data[3] = 0x03;
    adv_data[4] = GAP_AD_TYPE_COMPLETE_LIST_16_BIT_UUID; // 0x03
    #if (PRF_HIDS)
    write16p(&adv_data[5], 0x1812); // HID Service
    #else
    write16p(&adv_data[5], 0xFF00); // Serial Service
    #endif
    
    // Set appearance: 4B
    uint16_t icon = app_icon_get();
    adv_data[7] = 0x03;
    adv_data[8] = GAP_AD_TYPE_APPEARANCE; // 0x19
    write16p(&adv_data[9], icon);
#endif
    gapm_set_adv_data(actv_env.advidx, GAPM_SET_ADV_DATA, sys_config.adv_data_len, sys_config.adv_data);
}

static void app_adv_set_scan_rsp(void)
{
    uint8_t length;
    uint8_t rsp_data[DEV_NAME_MAX_LEN+2];
    
    // Set device name
    length = app_name_get(DEV_NAME_MAX_LEN, &rsp_data[2]);
    rsp_data[0] = length + 1;
    rsp_data[1] = GAP_AD_TYPE_COMPLETE_NAME; // 0x09

    gapm_set_adv_data(actv_env.advidx, GAPM_SET_SCAN_RSP_DATA, length + 2, rsp_data);
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
                gapm_start_advertising(actv_env.advidx, APP_ADV_DURATION);
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
        } break;
        
        default:
            break;
    }
}

#endif //(BLE_EN_ADV)


/**
 ****************************************************************************************
 * @section Scanning Activity - Example for User Customize
 *          enable via pre-define @see BLE_EN_SCAN
 ****************************************************************************************
 */
#if (BLE_EN_SCAN)


/*
 * DEFINITIONS
 ****************************************************************************************
 */

#define SCAN_NUM_MAX         (5)

uint8_t scan_cnt = 0;
struct gap_bdaddr scan_addr_list[SCAN_NUM_MAX];


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

static void app_start_scanning(void)
{
    struct gapm_scan_param scan_param;
    
    /// Type of scanning to be started (@see enum gapm_scan_type)
    scan_param.type = GAPM_SCAN_TYPE_CONN_DISC;
    /// Properties for the scan procedure (@see enum gapm_scan_prop)
    scan_param.prop = GAPM_SCAN_PROP_PHY_1M_BIT | GAPM_SCAN_PROP_ACTIVE_1M_BIT | GAPM_SCAN_PROP_FILT_TRUNC_BIT;
    /// Duplicate packet filtering policy (@see enum gapm_dup_filter_pol)
    scan_param.dup_filt_pol = GAPM_DUP_FILT_EN;
    /// Scan window opening parameters for LE 1M PHY (in unit of 625us)
    scan_param.scan_param_1m.scan_intv = GAP_SCAN_FAST_INTV;
    scan_param.scan_param_1m.scan_wd   = GAP_SCAN_FAST_WIND;
    /// Scan window opening parameters for LE Coded PHY
    //scan_param.scan_param_coded.scan_intv = GAP_SCAN_SLOW_INTV1;
    //scan_param.scan_param_coded.scan_wd   = GAP_SCAN_SLOW_WIND1;
    /// Scan duration (in unit of 10ms). 0 means that the controller will scan continuously until
    /// reception of a stop command from the application
    scan_param.duration = 500;//GAP_TMR_GEN_DISC_SCAN;
    /// Scan period (in unit of 1.28s). Time interval betweem two consequent starts of a scan duration
    /// by the controller. 0 means that the scan procedure is not periodic
    scan_param.period = 0;
    
    gapm_start_activity(actv_env.scanidx, sizeof(struct gapm_scan_param), &scan_param);
}

/**
 ****************************************************************************************
 * @brief Action/Command of Scanning
 *
 * @param[in] actv_op  Operation of activity
 ****************************************************************************************
 */
void app_scan_action(uint8_t actv_op)
{
    switch (actv_op)
    {
        case ACTV_CREATE:
        {
            if (actv_env.scansta == ACTV_STATE_OFF)
            {
                DEBUG("Creating");
                gapm_create_activity(GAPM_ACTV_TYPE_SCAN, GAPM_STATIC_ADDR);
                actv_env.scansta = ACTV_STATE_CREATE;
            }
        } break;
        
        case ACTV_START:
        {
            scan_cnt = 0;          
            
            if (actv_env.scansta == ACTV_STATE_READY)
            {
                DEBUG("Starting");
                app_start_scanning();
                actv_env.scansta = ACTV_STATE_START;
            }
        } break;
        
        case ACTV_STOP:
        {
            if (actv_env.scansta == ACTV_STATE_START)
            {
                DEBUG("Stopping");
                gapm_stop_activity(actv_env.scanidx);
                actv_env.scansta = ACTV_STATE_STOP;
            }
        } break;
        
        case ACTV_DELETE:
        {
            if ((actv_env.scansta != ACTV_STATE_OFF) && (actv_env.scansta != ACTV_STATE_START))
            {
                DEBUG("Deleting");
                gapm_delete_activity(actv_env.scanidx);
                actv_env.scansta = ACTV_STATE_OFF;
            }
        } break;
        
        case ACTV_RELOAD:
        {
            if (actv_env.scansta == ACTV_STATE_START)
            {
                DEBUG("Stopping");
                gapm_stop_activity(actv_env.scanidx);
                actv_env.scansta = ACTV_STATE_STOP;
            }
            
            if ((actv_env.scansta != ACTV_STATE_OFF) && (actv_env.scansta != ACTV_STATE_START))
            {
                DEBUG("Deleting");
                gapm_delete_activity(actv_env.scanidx);
                actv_env.scansta = ACTV_STATE_OFF;
            }
            
            DEBUG("Creating");
            gapm_create_activity(GAPM_ACTV_TYPE_SCAN, GAPM_STATIC_ADDR);
            actv_env.scansta = ACTV_STATE_CREATE;
        } break;
        
        default:
            break;
    }
}

/**
 ****************************************************************************************
 * @brief Event Procedure of Scanning 
 *
 * @param[in] gapm_op  Operation of gapm
 * @param[in] status   Status of event
 ****************************************************************************************
 */
void app_scan_event(uint8_t gapm_op, uint8_t status)
{
    DEBUG("Evt(op:0x%X,sta:0x%X)", gapm_op, status);
    
    switch (gapm_op)
    {
        case GAPM_CREATE_SCAN_ACTIVITY:
        {
            actv_env.scansta = ACTV_STATE_READY;
        } break;
        
        case GAPM_STOP_ACTIVITY:
        {
            if ((actv_env.scansta == ACTV_STATE_START) || (actv_env.scansta == ACTV_STATE_STOP))
            {
                actv_env.scansta = ACTV_STATE_READY;
            }
            
            DEBUG("-->Filter DevAddr");
            for (uint8_t idx = 0; idx < scan_cnt; idx++)
            {
                DEBUG("Scan List[%d]-->", idx);
                debugHex((uint8_t *)(&scan_addr_list[idx]), sizeof(struct gap_bdaddr));             
            }
        } break;
        
        default:
            break;
    }
}

/**
 ****************************************************************************************
 * @brief Store result of Scanning when filter by app_actv_report_ind
 *
 * @param[in] paddr    gap_bdaddr of peer device
 ****************************************************************************************
 */
void app_scan_result(const struct gap_bdaddr* paddr)
{
    for (uint8_t i = 0; i < scan_cnt; i++)
    {
        if (!memcmp(&scan_addr_list[i], paddr,sizeof(struct gap_bdaddr)))//save addr but diffrent
        {
            return;
        }                           
    }

    if (scan_cnt < SCAN_NUM_MAX) //get null array
    {
        memcpy(&scan_addr_list[scan_cnt], paddr,sizeof(struct gap_bdaddr));
        scan_cnt++;
    }                        
}

/**
 ****************************************************************************************
 * @brief Handles activity report. (@see GAPM_EXT_ADV_REPORT_IND)
 *
 * @param[in] report  Report of Advertising data be scanned
 ****************************************************************************************
 */
void app_actv_report_ind(struct gapm_ext_adv_report_ind const* report)
{
    // filter report
    if ((report->info & GAPM_REPORT_INFO_REPORT_TYPE_MASK) == GAPM_REPORT_TYPE_ADV_LEG)
    {
        const uint8_t *p_cursor = report->data;
        const uint8_t *p_end_cusor = report->data + report->length;
        
        while (p_cursor < p_end_cusor)
        {
            // Extract AD type
            uint8_t ad_type = *(p_cursor + 1);
            
            if (ad_type == GAP_AD_TYPE_APPEARANCE)
            {
                uint16_t icon = read16p(p_cursor+2);
                
                // Filter special appearance device
                if ((icon == 0x03C1) || (icon == 0x03C5)) // HID Gamepad
                {
                    app_scan_result(&report->trans_addr);
                    break;
                }
            }
            else if ((ad_type == GAP_AD_TYPE_COMPLETE_LIST_16_BIT_UUID) || (ad_type == GAP_AD_TYPE_MORE_16_BIT_UUID))
            {
                uint16_t uuid = read16p(p_cursor+2);
                
                // Filter special uuid device
                if (((uuid == 0x18F0) || (uuid == 0xFF00)))// && (param->trans_addr.addr.addr[0] == 0x04))
                {
                    app_scan_result(&report->trans_addr);
                    break;
                }
            }
            else
            {
                // Filter Rule more...
            }
            
            /* Go to next advertising info */
            p_cursor += (*p_cursor + 1);
        }
    }
}
#endif //(BLE_EN_SCAN)


/**
 ****************************************************************************************
 * @section Initiating Activity - Example for User Customize
 *          enable via pre-define @see BLE_EN_INIT
 ****************************************************************************************
 */
#if (BLE_EN_INIT)

#if !defined(APP_INIT_TIMEOUT)
#define APP_INIT_TIMEOUT      100 // timeout unit in 10ms
#endif


/*
 * DEFINITIONS
 ****************************************************************************************
 */

const struct gap_bdaddr dflt_peer = 
{
    .addr      = {{0x11, 0x22, 0x33, 0xa1, 0x01, 0xd3}},
    .addr_type = ADDR_RAND,
};

const struct gapm_conn_param dflt_conn_param = 
{
    /// Minimum value for the connection interval (in unit of 1.25ms). Allowed range is 7.5ms to 4s.
    .conn_intv_min = 10,
    /// Maximum value for the connection interval (in unit of 1.25ms). Allowed range is 7.5ms to 4s.
    .conn_intv_max = 10,
    /// Slave latency. Number of events that can be missed by a connected slave device
    .conn_latency = 0,
    /// Link supervision timeout (in unit of 10ms). Allowed range is 100ms to 32s
    .supervision_to = 300,
    /// Recommended minimum duration of connection events (in unit of 625us)
    .ce_len_min = 2,
    /// Recommended maximum duration of connection events (in unit of 625us)
    .ce_len_max = 4,
};


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Start initiating to peer device
 *
 * @param[in] paddr    gap_bdaddr of peer device
 ****************************************************************************************
 */
void app_start_initiating(const struct gap_bdaddr* paddr)
{
    if ((actv_env.initsta == ACTV_STATE_READY) || (actv_env.initsta == ACTV_STATE_STOP))
    {
        struct gapm_init_param init_param;

        if (paddr == NULL)
        {
            paddr = &dflt_peer;
        }
        
        init_param.type = GAPM_INIT_TYPE_DIRECT_CONN_EST;
        init_param.prop = GAPM_INIT_PROP_1M_BIT;
        init_param.conn_to = APP_INIT_TIMEOUT; // timeout unit in 10ms, update from v1.3
        init_param.scan_param_1m.scan_intv = GAP_SCAN_FAST_INTV;
        init_param.scan_param_1m.scan_wd   = GAP_SCAN_FAST_WIND;
        
        memcpy(&init_param.conn_param_1m, &dflt_conn_param, sizeof(struct gapm_conn_param));
        memcpy(&init_param.peer_addr, paddr, sizeof(struct gap_bdaddr));

        gapm_start_activity(actv_env.initidx, sizeof(struct gapm_init_param), &init_param);
        
        DEBUG("Starting");
        actv_env.initsta = ACTV_STATE_START;
    }
}

/**
 ****************************************************************************************
 * @brief Action/Command of Initiating
 *
 * @param[in] actv_op  Operation of activity
 ****************************************************************************************
 */
void app_init_action(uint8_t actv_op)
{  
    switch (actv_op)
    {
        case ACTV_CREATE:
        {
            if (actv_env.initsta == ACTV_STATE_OFF)
            {
                DEBUG("Creating");
                gapm_create_activity(GAPM_ACTV_TYPE_INIT, GAPM_STATIC_ADDR);
                actv_env.initsta = ACTV_STATE_CREATE;
            }
        } break;
        
        case ACTV_START:
        {
            app_start_initiating(NULL);
        } break;
        
        case ACTV_STOP:
        {
            if (actv_env.initsta == ACTV_STATE_START)
            {
                DEBUG("Stopping");
                gapm_stop_activity(actv_env.initidx);
                actv_env.initsta = ACTV_STATE_STOP;
            }
        } break;

        case ACTV_DELETE:
        {
            if ((actv_env.initsta != ACTV_STATE_OFF) && (actv_env.initsta != ACTV_STATE_START))
            {
                DEBUG("Deleting");
                gapm_delete_activity(actv_env.initidx);
                actv_env.initsta = ACTV_STATE_OFF;
            }
        } break;
        
        case ACTV_RELOAD:
        {
            if (actv_env.initsta == ACTV_STATE_START)
            {
                DEBUG("Stopping");
                gapm_stop_activity(actv_env.initidx);
                actv_env.initsta = ACTV_STATE_STOP;
            }
            
            if ((actv_env.initsta != ACTV_STATE_OFF) && (actv_env.initsta != ACTV_STATE_START))
            {
                DEBUG("Deleting");
                gapm_delete_activity(actv_env.initidx);
                actv_env.initsta = ACTV_STATE_OFF;
            }

            DEBUG("Creating");
            gapm_create_activity(GAPM_ACTV_TYPE_INIT, GAPM_STATIC_ADDR);
            actv_env.initsta = ACTV_STATE_CREATE;
        } break;
        
        default:
            break;
    }
}

/**
 ****************************************************************************************
 * @brief Event Procedure of Initiating 
 *
 * @param[in] gapm_op  Operation of gapm
 * @param[in] status   Status of event
 ****************************************************************************************
 */
void app_init_event(uint8_t gapm_op, uint8_t status)
{
    DEBUG("Evt(op:0x%X,sta:0x%X)", gapm_op, status);
    
    switch (gapm_op)
    {
        case GAPM_CREATE_INIT_ACTIVITY:
        {
            actv_env.initsta = ACTV_STATE_READY;
        } break;
        
        case GAPM_STOP_ACTIVITY:
        {
            if ((actv_env.initsta == ACTV_STATE_START) || (actv_env.initsta == ACTV_STATE_STOP))
            {
                actv_env.initsta = ACTV_STATE_READY;
            }
        } break;
        
        default:
            break;
    }
}

#endif //(BLE_EN_INIT)


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
    
    #if (BLE_EN_SCAN)
    app_scan_action(ACTV_CREATE);
    #endif //(BLE_EN_SCAN)
    
    #if (BLE_EN_INIT)
    app_init_action(ACTV_CREATE);
    #endif //(BLE_EN_INIT)
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
        
        #if (BLE_EN_SCAN)
        case GAPM_CREATE_SCAN_ACTIVITY:
        {
            app_scan_event(operation, status);
        } break;
        #endif //(BLE_EN_SCAN)
        
        #if (BLE_EN_INIT)
        case GAPM_CREATE_INIT_ACTIVITY:
        {
            app_init_event(operation, status);
        } break;
        #endif //(BLE_EN_INIT)
        
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
        } break;
        #endif //(BLE_EN_ADV)
        
        #if (BLE_EN_SCAN)
        case GAPM_ACTV_TYPE_SCAN:
        {
            actv_env.scanidx = actv_idx;
        } break;
        #endif //(BLE_EN_SCAN)
        
        #if (BLE_EN_INIT)
        case GAPM_ACTV_TYPE_INIT:
        {
            actv_env.initidx = actv_idx;
        } break;
        #endif //(BLE_EN_INIT)
        
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
        
        #if (BLE_EN_SCAN)
        case GAPM_ACTV_TYPE_SCAN:
        {
            app_scan_event(GAPM_STOP_ACTIVITY, reason);
        } break;
        #endif //(BLE_EN_SCAN)
        
        #if (BLE_EN_INIT)
        case GAPM_ACTV_TYPE_INIT:
        {
            app_init_event(GAPM_STOP_ACTIVITY, reason);
        } break;
        #endif //(BLE_EN_INIT)
        
        default:
            break;
    }
}
