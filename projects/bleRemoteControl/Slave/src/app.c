/**
 ****************************************************************************************
 *
 * @file app.c
 *
 * @brief Application entry point - Example
 *
 * < __weak func as demo, recommend to Override its in 'user porject'/src/myapp.c >
 ****************************************************************************************
 */

#include "app.h"
#include "bledef.h"
#include "drvs.h"
#include "prf_api.h"
#include "app_user.h"

#if (DBG_APP)
#include "dbg.h"
#define DEBUG(format, ...)    debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#endif


/*
 * DEFAULT CONFIGURATION
 ****************************************************************************************
 */

#if !defined(BLE_DEV_NAME)
    #define BLE_DEV_NAME        "myBle6"
#endif

#if !defined(BLE_ADDR)
    #define BLE_ADDR            { 0x30, 0x06, 0x23, 0x20, 0x01, 0xD2 }
#endif

#if !defined(BLE_ROLE)
#if (BLE_NB_SLAVE && BLE_NB_MASTER)
    #define BLE_ROLE            (GAP_ROLE_CENTRAL | GAP_ROLE_PERIPHERAL)
#elif (BLE_NB_MASTER)
    #define BLE_ROLE            (GAP_ROLE_CENTRAL)
#else  // Only Slave
    #define BLE_ROLE            (GAP_ROLE_PERIPHERAL)
#endif
#endif

#if !defined(BLE_PHY)
    #define BLE_PHY             (GAP_PHY_LE_1MBPS) // | GAP_PHY_LE_2MBPS)
#endif

#if !defined(BLE_PAIRING)
    #define BLE_PAIRING         (GAPM_PAIRING_LEGACY)
#endif

#if !defined(BLE_AUTH)
    #define BLE_AUTH            (GAP_AUTH_REQ_NO_MITM_NO_BOND)
#endif

#if !defined(BLE_SECREQ)
    #define BLE_SECREQ          (GAP_NO_SEC)
#endif


/*
 * VARIABLES DEFINITIONS
 ****************************************************************************************
 */

/// Application Environment
__VAR_ENV struct app_env_tag app_env;

/// Ble local address (user customize)
const bd_addr_t ble_dev_addr = { BLE_ADDR };

/// GAP device configuration
const struct gapm_dev_config ble_dev_config =
{
    // Device Role: Central, Peripheral (@see gap_role)
    .gap_role  = BLE_ROLE,

    // Pairing mode authorized (@see enum gapm_pairing_mode)
    .pairing   = BLE_PAIRING,

    // Preferred LE PHY for data (@see enum gap_phy)
    .pref_phy  = BLE_PHY,

    // Maximal MTU acceptable for device (23~512)
    .max_mtu   = BLE_MTU,
};

/// GAP debug LTK for testing
#if (BLE_DBG_LTK)
const struct gapc_ltk debugLTK = 
{
    /// Long Term Key
    .ltk = {{0x88, 0x0D, 0x00, 0x20, 0xAC, 0x32, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00}},
    /// Encryption Diversifier
    .ediv = 0xB951,
    /// Random Number
    .randnb = {{0x02, 0x18, 0xAC, 0x32, 0x00, 0x20, 0x00, 0x00}},
    /// Encryption key size (7 to 16)
    .key_size = 16,
    /// Extend Info
    .ext_info =0,
};
#endif


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @section Profile Interface
 ****************************************************************************************
 */

// Connection interval unit in 1.25ms
#define SLV_PREF_INTV_MIN       (SLV_INTV_MIN)
#define SLV_PREF_INTV_MAX       (SLV_INTV_MIN)
// Slave latency
#define SLV_PREF_LATENCY        (49)
// Connection supervision timeout multiplier unit in 10ms
#define SLV_PREF_TIME_OUT       (300)

const struct gapc_conn_param dft_conn_param = 
{
    .intv_min = SLV_PREF_INTV_MIN,
    .intv_max = SLV_PREF_INTV_MAX,
    .latency  = SLV_PREF_LATENCY,
    .time_out = SLV_PREF_TIME_OUT,
};
/**
 ****************************************************************************************
 * @brief Retrieve Dev Info to Generic Access Profile, User implement for Callback.
 *
 * @param[in]  conidx  connection index
 * @param[in]  req     iequest of info type @see enum gapc_dev_info
 * @param[in]  maxlen  buffer length, DEV_NAME_MAX_LEN or size of gapc_conn_param
 * @param[out] info    pointer of buffer
 *
 * @return Length of device information, 0 means an error occurs.
 ****************************************************************************************
 */
__weak uint16_t gap_svc_get_dev_info(uint8_t conidx, uint8_t req, uint16_t maxlen, uint8_t *info)
{
    if (req == GAPC_DEV_NAME)
    {
        return app_name_get(DEV_NAME_MAX_LEN, info);
    }
    else if (req == GAPC_DEV_APPEARANCE)
    {
        write16(info, app_icon_get());
        return sizeof(uint16_t);
    }
    #if (GAP_ATT_CFG & 0x40/*PCP_EN*/)
    else if (req == GAPC_DEV_SLV_PREF_PARAMS)
    {
        // Peripheral Preferred Connection Parameters
        memcpy(info, (uint8_t *)&dft_conn_param, sizeof(struct gapc_conn_param));
        return sizeof(struct gapc_conn_param);
    }
    #endif
    
    return 0;
}

/**
 ****************************************************************************************
 * @brief Create profiles, maybe User Override! (__weak func)
 *        Added in order and judged status in each profile-func.
 ****************************************************************************************
 */
__weak void app_prf_create(void)
{
    // Generic Access Profile(0x1800)
    gap_svc_init(GAP_START_HDL, GAP_ATT_CFG);
    
    // Generic Attribute Profile(0x1801)
    #if (BLE_NB_SLAVE)
    gatt_svc_init(GATT_START_HDL);
    #endif

    // Standard Profiles
    #if (PRF_DISS)
    diss_svc_init();
    #endif
    
    #if (PRF_BASS)
    bass_svc_init();
    #endif
    
    #if (PRF_HIDS)
    hids_prf_init();
    #endif

    #if (PRF_SCPS)
    scps_svc_init();
    #endif    

    #if (PRF_MESH)
    mal_svc_init(MAL_START_HDL, MAL_SVC_CFG);
    #endif

    // Customize Profiles
    #if (PRF_SESS)
    sess_svc_init();
    #endif

    #if (PRF_OTAS)
    otas_svc_init();
    #endif

    #if (PRF_PTSS)
    ptss_svc_init();
    #endif
}


/**
 ****************************************************************************************
 * @section App Interface
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief API to Init Application, maybe User Override! (__weak func)
 *
 * @param[in] rsn   reset reason @see enum rst_src_bfs
 ****************************************************************************************
 */
__weak void app_init(uint16_t rsn)
{
    // Init BLE and App to startup or Resume BLE to continue if it wakeup from poweroff.
    #if (BLE_LITELIB)
//    if (RSN_IS_BLE_WKUP(rsn))
    if ((rsn & RSN_POR12_CORE_BIT) && (AON->PMU_WKUP_CTRL.Word & CFG_WKUP_BLE_EN))
    {
        // Resume BLE (Only supported in LiteLib)
        ble_resume();
        rc32k_conf(RCLK_DPLL, 7);
        
        if (rsn & RSN_IO_WKUP_BIT)
        {
            ble_wakeup();
        }
        
        keys_proc();
    }
    else
    #endif //(BLE_LITELIB)
    {
        heap_cfg_t heap;
        
        // Config Heap, resized with special lib
        heap.base[MEM_ENV] = BLE_HEAP_BASE;
        heap.size[MEM_ENV] = BLE_HEAP_ENV_SIZE;
        heap.base[MEM_MSG] = BLE_HEAP_BASE + BLE_HEAP_ENV_SIZE;
        heap.size[MEM_MSG] = BLE_HEAP_MSG_SIZE;
        ble_heap(&heap);

        // Init BLE and App
        ble_init();
        ble_app();

        // Init RC32K with Calibration
        #if (CFG_SLEEP || RC32K_CALIB_PERIOD)
        //rc32k_init(); - replace to watch calib result
        rc32k_conf(RCLK_DPLL, 7);

        uint16_t trim = rc32k_calib();
        DEBUG("RC32K Calib(Msb:%d,Lsb:%d)", trim & 0xF, trim >> 4);
        #endif //(CFG_SLEEP || RC32K_CALIB_PERIOD)
        
        #if (CFG_SLEEP)
        ble_drift_set(1200);
        #endif
        
        adv_dir_flag = true;// 定向广播
    }

    // Init RF & Modem
    rfmdm_init();
    
    NVIC_EnableIRQ(BLE_IRQn);
}

/**
 ****************************************************************************************
 * @brief API to Set State of Application, maybe User Override! (__weak func)
 *
 * @param[in] state    new state
 ****************************************************************************************
 */
__weak void app_state_set(uint8_t state)
{
    DEBUG("State(old:%d,new:%d)", app_state_get(), state);

    app_env.state = state;
    
    // Indication, User add more...

}

/**
 ****************************************************************************************
 * @brief API to Get Device Name, maybe User Override! (__weak func)
 *
 * @param[in]  size   Length of name Buffer
 * @param[out] name   Pointer of name buffer
 *
 * @return Length of device name
 ****************************************************************************************
 */
__weak uint8_t app_name_get(uint8_t size, uint8_t *name)
{
    uint8_t BLE_DEV_NAME_A[] = BLE_DEV_NAME;
    
    uint8_t len = sizeof(BLE_DEV_NAME_A) - 1;

    memcpy(name, BLE_DEV_NAME_A, len);
    
    return len;
}


/**
 ****************************************************************************************
 * @section FSM Interface
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Finite state machine for Device Configure, maybe User Override! (__weak func)
 *
 * @param[in] evt   configure event @see enum ble_event
 ****************************************************************************************
 */
__weak void app_conf_fsm(uint8_t evt)
{
    if (evt == BLE_RESET)
    {
        memset(&app_env, 0, sizeof(app_env));
        
        // Set device config
        gapm_set_dev(&ble_dev_config, &ble_dev_addr, NULL);
    }
    else /*if (evt == BLE_CONFIGURED)*/
    {
        #if (CFG_SLEEP)
        // Set Sleep duration if need
        #if (RC32K_CALIB_PERIOD)
        ke_timer_set(APP_TIMER_RC32K_CORR, TASK_APP, RC32K_CALIB_PERIOD);
        #endif //(RC32K_CALIB_PERIOD)
        #endif //(CFG_SLEEP)
        
        app_state_set(APP_IDLE);
        // Create Profiles
        app_prf_create();
        
        if (findPairInfo())
        {
            // 配对信息存在, 进入广播模式
            // Create Activities
            app_actv_create();
            ke_timer_set(APP_TIMER_KEY_SCAN, TASK_APP, KEY_SCAN_PERIOD);
        }
        else
        {
            keys_proc();
        }

    }
}

/**
 ****************************************************************************************
 * @brief Finite state machine for connection event, maybe User Override! (__weak func)
 *
 * @param[in] evt     connection event @see enum ble_event
 * @param[in] conidx  connection index
 * @param[in] param   param of connection event
 ****************************************************************************************
 */
__weak void app_conn_fsm(uint8_t evt, uint8_t conidx, const void* param)
{
    switch (evt)
    {
        case BLE_CONNECTED:
        {
            // Connected state, record Index
            app_env.curidx = conidx;
            app_state_set(APP_CONNECTED);

            gatt_exmtu(conidx, 247);
            gapc_update_dle(app_env.curidx,247,LE_MAX_TIME);

            gapc_connect_rsp(conidx, BLE_AUTH);
            // Enable profiles by role
        } break;
        
        case BLE_DISCONNECTED:
        {
            rc32k_calib();
            
            app_state_set(APP_IDLE);
            
            struct gapc_disconnect_ind *dis_param = (struct gapc_disconnect_ind *)param;
//            if (dis_param->reason == LE_ERR_HL2HCI(LL_ERR_REMOTE_USER_TERM_CON) || 
//                dis_param->reason == LE_ERR_HL2HCI(LL_ERR_REMOTE_DEV_POWER_OFF))
            if (dis_param->reason == LE_ERR_HL2HCI(LL_ERR_CON_TERM_BY_LOCAL_HOST))
            {
//                app_adv_action(ACTV_START);            
                adv_dir_flag = true;// 定向广播
                app_adv_action(ACTV_RELOAD);
                
                app_state_set(APP_READY);
            }
            else
            {
                keys_sleep();
            }
        } break;
        
        case BLE_BONDED:
        {
            // todo, eg. save the generated slave's LTK to flash
            app_ltk_save(conidx, NULL);
            app_state_set(APP_BONDED);
        } break;
        
        case BLE_ENCRYPTED:
        {
            app_state_set(APP_ENCRYPTED);
            // todo
        } break;
        
        default:
            break;
    }
}


/**
 ****************************************************************************************
 * @section SMP Interface
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief API to Get Pairing Feature, maybe User Override! (__weak func)
 *
 * @param[out] feat   Pointer of pairing buffer to fill
 ****************************************************************************************
 */
__weak void app_pairing_get(struct gapc_pairing *feat)
{
    // IO capabilities (@see gap_io_cap)
    feat->iocap     = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
    // OOB information (@see gap_oob)
    feat->oob       = GAP_OOB_AUTH_DATA_NOT_PRESENT;
    // Authentication (@see gap_auth)
    feat->auth      = BLE_AUTH;
    // Encryption key size (7 to 16)
    feat->key_size  = GAP_KEY_LEN;
    //Initiator key distribution (@see gap_kdist)
    feat->ikey_dist = GAP_KDIST_NONE;
    //Responder key distribution (@see gap_kdist)
    feat->rkey_dist = GAP_KDIST_ENCKEY;
    // Device security requirements (@see gap_sec_req)
    feat->sec_req   = BLE_SECREQ;
}

/**
 ****************************************************************************************
 * @brief API to Generate LTK for bonding, maybe User Override! (__weak func)
 *
 * @param[in]     conidx   connection index
 * @param[in|out] ltk      Pointer of ltk buffer
 ****************************************************************************************
 */
__weak void app_ltk_gen(uint8_t conidx, struct gapc_ltk *ltk)
{
    // generate key values, User need record it to save later
    // ltk->ediv = (uint16_t)rand_word();
    // ltk->key_size = GAP_KEY_LEN;
    // ltk->ext_info = 0;
    // for (uint8_t i = 0; i < GAP_RAND_NB_LEN; i++)
    // {
    //     ltk->randnb.nb[i] = (uint8_t)rand_word();
    // }
    // for (uint8_t i = 0; i < GAP_KEY_LEN; i++)
    // {
    //     ltk->ltk.key[i] = (uint8_t)rand_word();
    // }

    #if (BLE_DBG_LTK)
    // (here use debugLTK as testing)
    memcpy(ltk, &debugLTK, sizeof(struct gapc_ltk));
    #endif
}

/**
 ****************************************************************************************
 * @brief API to Save LTK when bonded, maybe User Override! (__weak func)
 *
 * @param[in] conidx   connection index
 * @param[in] ltk      Pointer of LTK data
 ****************************************************************************************
 */
__weak void app_ltk_save(uint8_t conidx, const struct gapc_ltk *ltk)
{
    // todo, save slave's LTK to flash
}

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
__weak const uint8_t *app_ltk_find(uint16_t ediv, const uint8_t *rand_nb)
{
    // Compare ediv and rand_nb (here use debugLTK as testing)
    #if (BLE_DBG_LTK)
    if ((ediv == debugLTK.ediv)
        && (memcmp(rand_nb, debugLTK.randnb.nb, GAP_RAND_NB_LEN) == 0))
    {
        return debugLTK.ltk.key;
    }
    #endif

    // Not found
    return NULL;
}
