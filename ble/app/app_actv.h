/**
 ****************************************************************************************
 *
 * @file app_actv.h
 *
 * @brief Header file - Application Activity(Advertising, Scanning and Initiating).
 *
 ****************************************************************************************
 */

#ifndef APP_ACTV_H_
#define APP_ACTV_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include <stdint.h>
#include "string.h"
#include "gapm.h"
#include "gapc.h"


/*
 * DEFINES
 ****************************************************************************************
 */

/// Operation of Activity
enum actv_op
{
    // Activity create
    ACTV_CREATE            = 0,
    // Activity start
    ACTV_START,
    // Activity stop
    ACTV_STOP,
    // Activity delete
    ACTV_DELETE,
    // Activity reload
    ACTV_RELOAD,
};

/// States of Activity
enum actv_state
{
    // Activity disable
    ACTV_STATE_OFF         = 0,
    // Activity creating
    ACTV_STATE_CREATE,
    // Activity created
    ACTV_STATE_READY,
    // Activity started
    ACTV_STATE_START,
    // Activity stopping
    ACTV_STATE_STOP,
};

/// Configure of Activity - Used for PTS-Test
struct actv_conf
{
    // discovery mode (@see gapm_adv_disc_mode)
    uint8_t adv_disc;
    // adv properties (@see enum gapm_adv_prop)
    uint8_t adv_prop;
    // bit7~2:data type bit1~0:address type (@see gapm_own_addr)
    uint8_t adv_conf;

    // own address type as master role (@see gapm_own_addr)
    uint8_t mst_conf;
    // /// Scanning Types (@see enum gapm_scan_type)
    uint8_t sca_type;
};

extern uint8_t scan_cnt;
extern struct gap_bdaddr scan_addr_list[];

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Create activities when Initialization complete.
 ****************************************************************************************
 */
void app_actv_create(void);

/**
 ****************************************************************************************
 * @brief Handles activity command complete event.
 *
 * @param[in] gapm_op  Operation of gapm
 * @param[in] status   Status of event
 ****************************************************************************************
 */
void app_actv_cmp_evt(uint8_t gapm_op, uint8_t status);

/**
 ****************************************************************************************
 * @brief Handles activity created. (@see GAPM_ACTIVITY_CREATED_IND)
 *
 * @param[in] actv_type  Type of activity(@see enum gapm_actv_type)
 * @param[in] actv_idx   Index of activity created
 ****************************************************************************************
 */
void app_actv_created_ind(uint8_t actv_type, uint8_t actv_idx);

/**
 ****************************************************************************************
 * @brief Handles activity stopped. (@see GAPM_ACTIVITY_STOPPED_IND)
 *
 * @param[in] actv_type  Type of activity(@see enum gapm_actv_type)
 * @param[in] reason     Reason of stopped
 ****************************************************************************************
 */
void app_actv_stopped_ind(uint8_t actv_type, uint8_t actv_idx, uint8_t reason);

/**
 ****************************************************************************************
 * @brief Handles activity report. (@see GAPM_EXT_ADV_REPORT_IND)
 *
 * @param[in] report  Report of Advertising data be scanned
 ****************************************************************************************
 */
void app_actv_report_ind(struct gapm_ext_adv_report_ind const* report);


/**
 ****************************************************************************************
 * @brief Action/Command of Advertising
 *
 * @param[in] actv_op  Operation of activity
 ****************************************************************************************
 */
void app_adv_action(uint8_t actv_op);

/**
 ****************************************************************************************
 * @brief Action/Command of Scanning
 *
 * @param[in] actv_op  Operation of activity
 ****************************************************************************************
 */
void app_scan_action(uint8_t actv_op);

/**
 ****************************************************************************************
 * @brief Store result of Scanning when filter by app_actv_report_ind
 *
 * @param[in] paddr    gap_bdaddr of peer device
 ****************************************************************************************
 */
void app_scan_result(const struct gap_bdaddr* paddr);


/**
 ****************************************************************************************
 * @brief Action/Command of Initiating
 *
 * @param[in] actv_op  Operation of activity
 ****************************************************************************************
 */
void app_init_action(uint8_t actv_op);

/**
 ****************************************************************************************
 * @brief Start initiating to peer device
 *
 * @param[in] paddr    gap_bdaddr of peer device
 ****************************************************************************************
 */
void app_start_initiating(const struct gap_bdaddr* paddr);


#endif // APP_ACTV_H_
