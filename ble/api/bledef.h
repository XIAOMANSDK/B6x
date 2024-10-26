/**
 ****************************************************************************************
 *
 * @file bledef.h
 *
 * @brief Interface functions of ble stack
 *
 ****************************************************************************************
 */

#ifndef _BLEDEF_H_
#define _BLEDEF_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include <stdint.h>
#include <stdbool.h>

#include "blelib.h"
#include "ke_api.h"
#include "gapm_api.h"
#include "gapc_api.h"
#include "gatt_api.h"
#include "prf_api.h"


/*
 * DEFINES
 ****************************************************************************************
 */

/// Status of ble
enum ble_status
{
    /// ble in active
    BLE_IN_ACTIVE    = 0,
    /// ble in idle, sys can be put in sleep
    BLE_IN_IDLE      = 1,
    /// ble in sleeping, core can deep sleep soon
    BLE_IN_SLEEP     = 2,
    /// ble in deep sleep, discard status
    BLE_IN_DEEPSL    = BLE_IN_SLEEP,
    /// ble in wakeup, correction ongoing.
    BLE_IN_WAKEUP    = 4,
};

/// Default LP Cycles of ble_sleep(), unit in 31.25us
#define BLE_SLP_TWOSC           (64)     // 2ms
#define BLE_SLP_DURMAX          (0x7D00) // 1s

/// Judge BLE Lib valid or not, if need
#define BLE_LIB_IS_RIGHT()      (ble_exch_size() <= BLE_EXCH_SIZE)


/*
 * FUNCTION DECLARATION
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Configure Heap of Ble Stack
 *
 ****************************************************************************************
 */
void ble_heap(heap_cfg_t *cfg);

/**
 ****************************************************************************************
 * @brief Initializes Ble Stack.
 *
 ****************************************************************************************
 */
void ble_init(void);

/**
 ****************************************************************************************
 * @brief Initializes RF & Modem.
 *
 ****************************************************************************************
 */
void rfmdm_init(void);

/**
 ****************************************************************************************
 * @brief Set RF Tx PA Level.
 *
 * @param[in] pa_target PA Level(0x00 ~ 0x0F). Default(0x0F).
 ****************************************************************************************
 */
void rf_pa_set(uint8_t pa_target);

/**
 ****************************************************************************************
 * @brief Reset link layer and the host
 *
 * @return Message GAPM_CMP_EVT(operation GAPM_RESET)
 ****************************************************************************************
 */
void ble_reset(void);

/**
 ****************************************************************************************
 * @brief Init Task of Application, interval call gapm_reset() to start.
 *
 ****************************************************************************************
 */
void ble_app(void);

/**
 ****************************************************************************************
 * @brief Schedule all pending events.
 *
 ****************************************************************************************
 */
void ble_schedule(void);

#if (BLE_LITELIB)
/**
 ****************************************************************************************
 * @brief Resume BLE quickly to continue run.
 *
 ****************************************************************************************
 */
void ble_resume(void);
#endif //(BLE_LITELIB)

/**
 ****************************************************************************************
 * @brief Wakeup BLE from sleep state.
 *
 * @return LP Cycles of sleeped time when wakeup enabled, 0 means not in sleep state.
 ****************************************************************************************
 */
uint32_t ble_wakeup(void);

/**
 ****************************************************************************************
 * @brief BLE try to enter sleep mode.
 *
 * @param[in] twosc    LP Cycles of oscillator stabilization when exit sleep state.
 * @param[in] dur_max  LP Cycles of max deepsltime, 0 means result of ble_slpdur_get() used.
 *
 * @return ble status (@see enum ble_status)
 ****************************************************************************************
 */
uint8_t ble_sleep(uint16_t twosc, uint32_t dur_max);

/**
 ****************************************************************************************
 * @brief Get BLE current time.
 *
 * @return  half-slot time(unit in 312.5us)
 ****************************************************************************************
 */
uint32_t ble_time_get(void);

/**
 ****************************************************************************************
 * @brief Get max sleep time when BLE is idle state.
 *
 * @return  LP Cycles of sleep time(unit in 31.25us), 0 means prevent sleep
 ****************************************************************************************
 */
uint32_t ble_slpdur_get(void);

/**
 ****************************************************************************************
 * @brief Get ble6*.lib version.
 *
 * @return  Version Info Struct:
 *              Byte0 Maximum Actvs - (ACT_MAX<<4) | CON_MAX, eg. 0x43=4ACT3CON
 *              Byte1 Support Feats - b[4]: Mesh, b[3:0]: Peri|Central|Broadcast|Observer
 *              Byte2 Build Version - eg. 0
 *              Byte3 Major Version - eg. 0x13=v1.3
 ****************************************************************************************
 */
uint32_t ble_lib_vers(void);

/**
 ****************************************************************************************
 * @brief Get memory size of Exchange with BLE HW.
 *
 * @return  size in bytes, aligned 4
 ****************************************************************************************
 */
uint32_t ble_exch_size(void);

/**
 ****************************************************************************************
 * @brief Config SLAVE_ROLE ACL More Data(MD)
 *
 * @param[in] en  set 1 to allow receiving the ACK in the same event, 0 to disable(default).
 *
 ****************************************************************************************
 */
void ble_txmd_set(uint8_t en);

/**
 ****************************************************************************************
 * @brief Config LPCLK(RC32K) Drift
 *
 * @param[in] ppm  the drift of RC32K, defalut 500ppm.
 *
 ****************************************************************************************
 */
void ble_drift_set(uint16_t ppm);

/**
 ****************************************************************************************
 * @brief Set Sync Word for BLE-2.4G Protocol.
 *
 * @param[in] syncw  Sync Word
 ****************************************************************************************
 */
void ble_2G4_set(uint16_t sync_l, uint16_t sync_h);

/**
 ****************************************************************************************
 * @brief Set Slave applied latency or not.
 *
 * @param[in] applied  True applied or False no applied.
 ****************************************************************************************
 */
void ble_latency_applied(bool applied);

/**
 ****************************************************************************************
 * @brief Set channel selection algorithm #2 is supported or not.
 *
 * @param[in] en  set 1 to allow channel selection algorithm #2 is supported, 0 to disable(default).
 ****************************************************************************************
 */
void ble_txchsel2_set(uint8_t en);
#endif // _BLEDEF_H_
