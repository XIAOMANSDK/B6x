/**
 ****************************************************************************************
 *
 * @file prf_bass.h
 *
 * @brief Header file - Battery Service Server Role
 *
 ****************************************************************************************
 */

#ifndef PRF_BASS_H_
#define PRF_BASS_H_

#include <stdint.h>


/*
 * DEFINES
 ****************************************************************************************
 */

/// Server Start Handle(0 = dynamically allocated)
#if !defined(BAS_START_HDL)
    #define BAS_START_HDL           (0)
#endif

/// Support Battery Power State Characteristic
#if !defined(BAS_PWR_STA)
    #define BAS_PWR_STA             (0)
#endif


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Add Battery Service Profile in the DB.
 *        Customize via pre-define @see BAS_START_HDL
 *
 * @return Result status, LE_SUCCESS or Error Reason
 ****************************************************************************************
 */
uint8_t bass_svc_init(void);

/**
 ****************************************************************************************
 * @brief Update Battery Level value
 *
 * @param[in] bat_lvl    Battery Level
 ****************************************************************************************
 */
void bass_bat_lvl_update(uint8_t bat_lvl);

/**
 ****************************************************************************************
 * @brief Enable Battery Level Notification Configurations
 *
 * @param[in] conidx     Connection index
 * @param[in] ntf_cfg    Notification Config @see prf_cli_conf
 ****************************************************************************************
 */
void bass_set_lvl_ntf(uint8_t conidx, uint8_t ntf_cfg);

#if (BAS_PWR_STA)
/**
 ****************************************************************************************
 * @brief Update Battery Power State value
 *
 * @param[in] pwr_sta    Power State
 ****************************************************************************************
 */
void bass_pwr_sta_update(uint8_t pwr_sta);

/**
 ****************************************************************************************
 * @brief Enable Battery Power State Notification Configurations
 *
 * @param[in] conidx     Connection index
 * @param[in] ntf_cfg    Notification Config @see prf_cli_conf
 ****************************************************************************************
 */
void bass_set_pwr_ntf(uint8_t conidx, uint8_t ntf_cfg);
#else
#define bass_pwr_sta_update(pwr_sta)
#define bass_set_pwr_ntf(conidx, ntf_cfg)
#endif


#endif /* PRF_BASS_H_ */
