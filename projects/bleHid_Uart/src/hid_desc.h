/**
 ****************************************************************************************
 *
 * @file hid_desc.h
 *
 * @brief Header file - HID Report Map, Descriptions.
 *
 * < Implementation according to user's application-specific >
 ****************************************************************************************
 */

#ifndef HID_DESC_H_
#define HID_DESC_H_

#include <stdint.h>


/*
 * DEFINES
 ****************************************************************************************
 */

#define HID_BOOT_SUP                (0)
#define HID_REPORT_SUP              (1)

#if (HID_BOOT_SUP)
    #define HID_BOOT_KB             (0)
    #define HID_BOOT_MOUSE          (0)
#endif //(HID_BOOT_SUP)

#if (HID_REPORT_SUP)
    #define HID_RPT_KB              (1)
    #define HID_RPT_MEDIA           (1)
    #define HID_RPT_SYSTEM          (1)
    #define HID_RPT_MOUSE           (1)
    #define HID_RPT_PTP             (0)
#endif //(HID_REPORT_SUP)

/// HID Report ID and Length, declared in Report Map (User Customize)
#define RPT_ID_KB                   (1)
#define RPT_LEN_KB                  (8)  // 1B(ctlkeys) + 1B(resv) + 6B(keycode)

#define RPT_ID_MEDIA                (2)
#define RPT_LEN_MEDIA               (2)  // 16bits

#define RPT_ID_SYSTEM               (3)
#define RPT_LEN_SYSTEM              (1)  // 8bits

#define RPT_ID_MOUSE                (4)
#if (XY_nB == 3)
    // XY 4B
    #define RPT_LEN_MOUSE           (10)  // 1B(button) + 4B(X) + 4B(Y) + 1B(Wheel)
#elif (XY_nB == 2)
    // XY 2B
    #define RPT_LEN_MOUSE           (6)  // 1B(button) + 2B(X) + 2B(Y) + 1B(Wheel)
#else
    // XY 1B
    #define RPT_LEN_MOUSE           (4)  // 1B(button) + 1B(X) + 1B(Y) + 1B(Wheel)
#endif

#define RPT_ID_TP                   (5)
#define RPT_ID_MAXCNT               (6)  // Feature - Finger count
#define RPT_ID_PTPHQA               (7)  // Feature - HQA
#define RPT_LEN_TP                  (29) // Touch point
#define PTP_MAX_FINGER_CNT          (5)

/// HID Report Index of Notification
#define RPT_NTF_STOP                (0x00)
#define RPT_NTF_ALL                 ((1 << RPT_IDX_NB) - 1)

enum rpt_ntf_idx
{
    // Indexes of HID BOOT
    RPT_IDX_BOOT_KB,
    RPT_IDX_BOOT_MOUSE,
    // Indexes of HID REPORT
    RPT_IDX_KB,
    RPT_IDX_MEDIA,
    RPT_IDX_SYSTEM,
    RPT_IDX_MOUSE,
    RPT_IDX_TP,

    // Max Index, *NOTE* not excced Bits of rpt_ntfs
    RPT_IDX_NB,
};


/*
 * MACRO DEFINITIONS
 ****************************************************************************************
 */

#include "prf_hids.h"

#define mouse_report_send(conidx, p_data)   hids_report_send(conidx, RPT_IDX_MOUSE,  RPT_LEN_MOUSE,  p_data)
#define keybd_report_send(conidx, p_data)   hids_report_send(conidx, RPT_IDX_KB,     RPT_LEN_KB,     p_data)
#define media_report_send(conidx, p_data)   hids_report_send(conidx, RPT_IDX_MEDIA,  RPT_LEN_MEDIA,  p_data)
#define system_report_send(conidx, p_data)  hids_report_send(conidx, RPT_IDX_SYSTEM, RPT_LEN_SYSTEM, p_data)


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Get HID Report Map description.
 *
 * @param[out] plen  Retrieved Length of description
 * 
 * @return Pointer of the description
 ****************************************************************************************
 */
const uint8_t *hid_get_report_map(uint16_t *plen);

#if (HID_RPT_PTP)
/**
 ****************************************************************************************
 * @brief Get HID PTP HQA description.
 *
 * @param[out] plen  Retrieved Length of description
 * 
 * @return Pointer of the description
 ****************************************************************************************
 */
const uint8_t *hid_get_ptphqa_blob(uint16_t *plen);
#endif //(HID_RPT_PTP)


#endif /* HID_DESC_H_ */
