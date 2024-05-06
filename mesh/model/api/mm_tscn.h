/**
 ****************************************************************************************
 *
 * @file mm_sens.h
 *
 * @brief Header file for Mesh Time and Scene Model Definitions
 *
 ****************************************************************************************
 */

#ifndef MM_TSCN_H_
#define MM_TSCN_H_

/**
 ****************************************************************************************
 * @defgroup MM_DEFINES Mesh Model Definitions
 * @ingroup MESH_MDL
 * @brief  Mesh Model Defines
 * @{
 ****************************************************************************************
 */

/*
 * DEFINES (MODEL IDS)
 ****************************************************************************************
 */

/// ********************** Model IDs for Time and Scenes Models *************************

/// Time and Scene Server - Time
#define MM_ID_TSCNS_TIM         (0x1200)
/// Time and Scene Server - Time Setup
#define MM_ID_TSCNS_TIMS        (0x1201)
/// Time and Scene Server - Scene
#define MM_ID_TSCNS_SCN         (0x1203)
/// Time and Scene Server - Scene Setup
#define MM_ID_TSCNS_SCNS        (0x1204)
/// Time and Scene Server - Scheduler
#define MM_ID_TSCNS_SCH         (0x1206)
/// Time and Scene Server - Scheduler Setup
#define MM_ID_TSCNS_SCHS        (0x1207)

/// Time and Scene Client - Time
#define MM_ID_TSCNC_TIM         (0x1202)
/// Time and Scene Client - Scene
#define MM_ID_TSCNC_SCN         (0x1205)
/// Time and Scene Client - Scheduler
#define MM_ID_TSCNC_SCH         (0x1208)

/*
 * DEFINES
 ****************************************************************************************
 */

/*
 * DEFINES (MESSAGE LENGTH)
 ****************************************************************************************
 */

/*
 * ENUMERATIONS (MESSAGE CONTENT)
 ****************************************************************************************
 */

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// @} MM_DEFINES

#endif /* MM_TSCN_H_ */
