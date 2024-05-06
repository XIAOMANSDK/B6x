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

/// Server Start Handle(0 = dynamically allocated)
#if !defined(HID_START_HDL)
    #define HID_START_HDL           (0)
#endif

/// Number of reports that can be sent
#if !defined(HID_NB_PKT_MAX)
    #define HID_NB_PKT_MAX          (10)
#endif

/// Maximal length of Report/Boot Char. Value
#if !defined(HID_REPORT_MAX_LEN)
    #define HID_REPORT_MAX_LEN      (256)
#endif

/// Size of HID BOOT keyboard/mouse report
#define HID_BOOT_REPORT_MAX_LEN     (8)
#define HID_BOOT_KB_RPT_SIZE        (8) // 1B(ctrlKeys) + 1B(resv0) + 6B(KeyCode) 
#define HID_BOOT_MOUSE_RPT_SIZE     (4) // 1B(button) + 1B(X) + 1B(Y) + 1B(Wheel)
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
#define HID_INFO_BCDHID             (0x0000) // HID Version 1.11
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


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

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
 * @brief Show LED Lock of Keyboard Output, User Implement! (__weak func)
 *
 * @param[in] leds  Bits of Led_Lock(b0:num,b1:caps,b2:scroll)
 ****************************************************************************************
 */
void hids_led_lock(uint8_t leds);

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


#endif /* PRF_HIDS_H_ */
