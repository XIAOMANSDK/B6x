/**
 ****************************************************************************************
 *
 * @file hid_desc.c
 *
 * @brief HID Report Map, descriptions.
 *
 * < Implementation according to user's application-specific >
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "hid_desc.h"


/*
 * DEFINITIONS
 ****************************************************************************************
 */

/// HID Report Map description
const uint8_t hid_report_map[] =
{
    #if (HID_RPT_KB)
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x06,        // Usage (Keyboard)
    0xA1, 0x01,        // Collection (Application)
    0x85, RPT_ID_KB,   //   Report ID
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x08,        //   Report Count (8)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x19, 0xE0,        //   Usage Minimum (0xE0)
    0x29, 0xE7,        //   Usage Maximum (0xE7)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x03,        //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x06,        //   Report Count (6)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x19, 0x00,        //   Usage Minimum (0x00)
    0x29, 0xFF,        //   Usage Maximum (0xFF)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x05,        //   Report Count (5)
    0x05, 0x08,        //   Usage Page (LEDs)
    0x19, 0x01,        //   Usage Minimum (Num Lock)
    0x29, 0x05,        //   Usage Maximum (Kana)
    0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x75, 0x03,        //   Report Size (3)
    0x95, 0x01,        //   Report Count (1)
    0x91, 0x03,        //   Output (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,              // End Collection
    #endif //(HID_RPT_KB)

    #if (HID_RPT_MEDIA)
    0x05, 0x0C,        // Usage Page (Consumer)
    0x09, 0x01,        // Usage (Consumer Control)
    0xA1, 0x01,        // Collection (Application)
    0x85, RPT_ID_MEDIA,//   Report ID
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x10,        //   Report Count (16)
    
    0x09, 0x6F,        //   Usage (Display Brightness Increment)
    0x09, 0x70,        //   Usage (Display Brightness Decrement)
    0x09, 0xE9,        //   Usage (Volume Increment)
    0x09, 0xEA,        //   Usage (Volume Decrement)
    0x09, 0xB5,        //   Usage (Scan Next Track)
    0x09, 0xB6,        //   Usage (Scan Previous Track)
    0x09, 0xB7,        //   Usage (Stop)
    0x09, 0xCD,        //   Usage (Play/Pause)
    
    0x09, 0xE2,        //   Usage (Mute)
    0x0A, 0x21, 0x02,  //   Usage (AC Search)
    0x0A, 0x92, 0x01,  //   Usage (AL Calculator)
    0x0A, 0x24, 0x02,  //   Usage (AC Back)
    0x0A, 0x8A, 0x01,  //   Usage (AL Email Reader)
    0x0A, 0xAE, 0x01,  //   Usage (AL Keyboard Layout)
    0x0A, 0x23, 0x02,  //   Usage (AC Home)
    0x0A, 0x96, 0x01,  //   Usage (AL Internet Browser)
    
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              // End Collection
    #endif //(HID_RPT_MEDIA)

    #if (HID_RPT_SYSTEM)
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x80,        // Usage (Sys Control)
    0xA1, 0x01,        // Collection (Application)
    0x85, RPT_ID_SYSTEM,//   Report ID
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x03,        //   Report Count (3)
    0x09, 0x81,        //   Usage (Sys Power Down)
    0x09, 0x82,        //   Usage (Sys Sleep)
    0x09, 0x83,        //   Usage (Sys Wake Up)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x05,        //   Report Count (5)
    0x81, 0x03,        //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              // End Collection
    #endif //(HID_RPT_SYSTEM)
};


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
const uint8_t *hid_get_report_map(uint16_t *plen)
{
    *plen = sizeof(hid_report_map);
    
    return hid_report_map;
}
