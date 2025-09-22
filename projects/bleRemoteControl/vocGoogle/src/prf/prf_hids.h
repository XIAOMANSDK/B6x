/**
 ****************************************************************************************
 *
 * @file prf_hids.h
 *
 * @brief Header file - HID Service Server Role
 *
 ****************************************************************************************
 */

#ifndef PRF_HIDS_H_
#define PRF_HIDS_H_

#include <stdint.h>


/*
 * DEFINES
 ****************************************************************************************
 */
/// HID Report ID and Length, declared in Report Map (User Customize)
#define RPT_ID_KB                   (1)
#define RPT_LEN_KB                  (8)  // 1B(ctlkeys) + 1B(resv) + 6B(keycode)

#define RPT_ID_MEDIA                (2)
#define RPT_LEN_MEDIA               (4)  // 16bits

/// HID Report Index of Notification
#define RPT_NTF_STOP                (0x00)
#define RPT_NTF_ALL                 ((1 << RPT_IDX_NB) - 1)

enum rpt_ntf_idx
{
    // Indexes of HID BOOT
    RPT_IDX_BOOT_KB,

    // Indexes of HID REPORT
    RPT_IDX_KB,

    RPT_IDX_MEDIA,

//    RPT_IDX_FEATURE2_INPUT,
//    RPT_IDX_FEATURE3_INPUT,
//    RPT_IDX_FEATURE4_INPUT,

    // Max Index, *NOTE* not excced Bits of rpt_ntfs
    RPT_IDX_NB,
};

/// Server Start Handle(0 = dynamically allocated)
#define HID_START_HDL           (31) /*!< 包含service */
/// Number of reports that can be sent
#define HID_NB_PKT_MAX          (40)
/// Maximal length of Report/Boot Char. Value
#define HID_REPORT_MAX_LEN      (256)

/// Size of HID BOOT keyboard/mouse report
#define HID_BOOT_REPORT_MAX_LEN     (8)
#define HID_BOOT_KB_RPT_SIZE        (8) // 1B(ctrlKeys) + 1B(resv0) + 6B(KeyCode) 
/// Size of HID keyboard Output report
#define HID_KB_OUT_RPT_SIZE         (1) // 1B(leds)

/// Size of HID Information @see struct hid_info_tag
#define HID_INFO_SIZE               (4)
/// Size of HID Control Point @see enum hid_ctrl_pt
#define HID_CTRL_PT_SIZE            (1)
/// Size of HID Protocol Mode @see enum hid_proto_mode
#define HID_PROTO_MODE_SIZE         (1)
/// Size of HID Report Ref. @see struct hid_report_ref
#define HID_REPORT_REF_SIZE         (2)

/// Maximal length of Report Map Char. Value
#define HID_REPORT_MAP_MAX_LEN    (512)

/// HID Information, default value
#define HID_INFO_BCDHID             (0x1101) // HID Version 1.11
#define HID_INFO_BCODE              (0x00)   // bCountryCode
#define HID_INFO_FLAGS              (HID_REMOTE_WAKE_CAPABLE | HID_NORM_CONNECTABLE)


/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// HID Information flags
enum hid_info_flags
{
    /// Device capable of providing wake-up signal to a HID host
    HID_REMOTE_WAKE_CAPABLE   = 0x01,
    /// Normally connectable support bit
    HID_NORM_CONNECTABLE      = 0x02,
};

/// Protocol Mode Char. value Keys
enum hid_proto_mode
{
    /// Boot Protocol Mode
    HID_BOOT_PROTOCOL_MODE    = 0x00,
    /// Report Protocol Mode
    HID_REPORT_PROTOCOL_MODE  = 0x01,
};

/// HID Control Point Characteristic value keys
enum hid_ctrl_pt
{
    /// Suspend
    HID_CTRL_PT_SUSPEND       = 0x00,
    /// Exit suspend
    HID_CTRL_PT_EXIT_SUSPEND  = 0x01,
};

/// Report Reference Char. Configuration Values
enum hid_report_type
{
    /// Input Report(RD & NTF)
    HID_REPORT_INPUT          = 0x01,
    /// Output Report(RD & WR & WC)
    HID_REPORT_OUTPUT         = 0x02,
    /// Feature Report(RD & WR)
    HID_REPORT_FEATURE        = 0x03,
};

/// HID Information structure
struct hid_info_tag
{
    /// bcdHID
    uint16_t bcdHID;
    /// bCountryCode
    uint8_t  bCountryCode;
    /// Flags
    uint8_t  flags;
};

/// Report Reference structure
struct hid_report_ref
{
    /// Report ID
    uint8_t report_id;
    /// Report Type
    uint8_t report_type;
};

/**
 ****************************************************************************************
 * @brief Add HID Service Profile in the DB
 *        Customize via pre-define @see HID_START_HDL
 *
 * @return Result status, LE_SUCCESS or Error Reason
 ****************************************************************************************
 */
uint8_t hids_prf_init(void);

/**
 ****************************************************************************************
 * @brief Enable HID Notification Configurations.
 *
 * @param[in] conidx    Connection index
 * @param[in] rpt_ntf   Notification Config Bits @see enum rpt_ntf_idx.
 ****************************************************************************************
 */
void hids_set_ccc(uint8_t conidx, uint8_t rpt_ntf);

/**
 ****************************************************************************************
 * @brief Send HID Report to Host peer.
 *
 * @param[in] conidx   Connection Index
 * @param[in] rep_idx  Report Index
 * @param[in] rep_len  Report Length
 * @param[in] rep_val  Report Value
 * @return Status of the operation @see prf_err
 ****************************************************************************************
 */
uint8_t hids_report_send(uint8_t conidx, uint8_t rep_idx, uint16_t rep_len, const uint8_t* rep_val);

// HID按键report接口
int ble_hid_kb_report_send(uint16_t keycode);
#endif /* PRF_HIDS_H_ */
