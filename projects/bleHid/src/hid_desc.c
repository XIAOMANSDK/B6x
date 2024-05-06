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
    0x09, 0x70,        //   Usage (Display Brightness Decrement )
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

    #if (HID_RPT_MOUSE)
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x02,        // Usage (Mouse)
    0xA1, 0x01,        // Collection (Application)
    0x85, RPT_ID_MOUSE,//   Report ID
    0x09, 0x01,        //   Usage (Pointer)
    0xA1, 0x00,        //   Collection (Physical)
    0x05, 0x09,        //     Usage Page (Button)
    0x19, 0x01,        //     Usage Minimum (0x01)
    0x29, 0x08,        //     Usage Maximum (0x08)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0x01,        //     Logical Maximum (1)
    0x75, 0x01,        //     Report Size (1)
    0x95, 0x08,        //     Report Count (8)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
    0x16, 0x08, 0xFF,  //     Logical Minimum (-248)
    0x26, 0xFF, 0x00,  //     Logical Maximum (255)
    0x75, 0x10,        //     Report Size (16)
    0x95, 0x02,        //     Report Count (2)
    0x09, 0x30,        //     Usage (X)
    0x09, 0x31,        //     Usage (Y)
    0x81, 0x06,        //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0x15, 0x81,        //     Logical Minimum (-127)
    0x25, 0x7F,        //     Logical Maximum (127)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x01,        //     Report Count (1)
    0x09, 0x38,        //     Usage (Wheel)
    0x81, 0x06,        //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              //   End Collection
    0xC0,              // End Collection
    #endif //(HID_RPT_MOUSE)

    #if (HID_RPT_PTP)
    0x05, 0x0D,        // Usage Page (Digitizer)
    0x09, 0x05,        // Usage (Touch Pad)
    0xA1, 0x01,        // Collection (Application)
    0x85, RPT_ID_TP,   //   Report ID
    0x05, 0x0D,        //   Usage Page (Digitizer)
    0x09, 0x22,        //   Usage (Finger)
    0xA1, 0x02,        //   Collection (Logical)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0x01,        //     Logical Maximum (1)
    0x09, 0x47,        //     Usage (0x47)
    0x09, 0x42,        //     Usage (Tip Switch)
    0x95, 0x02,        //     Report Count (2)
    0x75, 0x01,        //     Report Size (1)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x01,        //     Report Count (1)
    0x75, 0x03,        //     Report Size (3)
    0x25, 0x05,        //     Logical Maximum (5)
    0x09, 0x51,        //     Usage (0x51)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x75, 0x01,        //     Report Size (1)
    0x95, 0x03,        //     Report Count (3)
    0x81, 0x03,        //     Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
    0x15, 0x00,        //     Logical Minimum (0)
    0x26, 0x54, 0x0B,  //     Logical Maximum (2900)
    0x75, 0x10,        //     Report Size (16)
    0x55, 0x0E,        //     Unit Exponent (-2)
    0x65, 0x11,        //     Unit (System: SI Linear, Length: Centimeter)
    0x09, 0x30,        //     Usage (X)
    0x35, 0x00,        //     Physical Minimum (0)
    0x46, 0x06, 0x04,  //     Physical Maximum (1030)
    0x95, 0x01,        //     Report Count (1)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x46, 0x49, 0x02,  //     Physical Maximum (585)
    0x26, 0xA4, 0x06,  //     Logical Maximum (1700)
    0x09, 0x31,        //     Usage (Y)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              //   End Collection
    0x05, 0x0D,        //   Usage Page (Digitizer)
    0x09, 0x22,        //   Usage (Finger)
    0xA1, 0x02,        //   Collection (Logical)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0x01,        //     Logical Maximum (1)
    0x09, 0x47,        //     Usage (0x47)
    0x09, 0x42,        //     Usage (Tip Switch)
    0x95, 0x02,        //     Report Count (2)
    0x75, 0x01,        //     Report Size (1)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x01,        //     Report Count (1)
    0x75, 0x03,        //     Report Size (3)
    0x25, 0x05,        //     Logical Maximum (5)
    0x09, 0x51,        //     Usage (0x51)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x75, 0x01,        //     Report Size (1)
    0x95, 0x03,        //     Report Count (3)
    0x81, 0x03,        //     Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
    0x15, 0x00,        //     Logical Minimum (0)
    0x26, 0x54, 0x0B,  //     Logical Maximum (2900)
    0x75, 0x10,        //     Report Size (16)
    0x55, 0x0E,        //     Unit Exponent (-2)
    0x65, 0x11,        //     Unit (System: SI Linear, Length: Centimeter)
    0x09, 0x30,        //     Usage (X)
    0x35, 0x00,        //     Physical Minimum (0)
    0x46, 0x06, 0x04,  //     Physical Maximum (1030)
    0x95, 0x01,        //     Report Count (1)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x46, 0x49, 0x02,  //     Physical Maximum (585)
    0x26, 0xA4, 0x06,  //     Logical Maximum (1700)
    0x09, 0x31,        //     Usage (Y)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              //   End Collection
    0x05, 0x0D,        //   Usage Page (Digitizer)
    0x09, 0x22,        //   Usage (Finger)
    0xA1, 0x02,        //   Collection (Logical)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0x01,        //     Logical Maximum (1)
    0x09, 0x47,        //     Usage (0x47)
    0x09, 0x42,        //     Usage (Tip Switch)
    0x95, 0x02,        //     Report Count (2)
    0x75, 0x01,        //     Report Size (1)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x01,        //     Report Count (1)
    0x75, 0x03,        //     Report Size (3)
    0x25, 0x05,        //     Logical Maximum (5)
    0x09, 0x51,        //     Usage (0x51)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x75, 0x01,        //     Report Size (1)
    0x95, 0x03,        //     Report Count (3)
    0x81, 0x03,        //     Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
    0x15, 0x00,        //     Logical Minimum (0)
    0x26, 0x54, 0x0B,  //     Logical Maximum (2900)
    0x75, 0x10,        //     Report Size (16)
    0x55, 0x0E,        //     Unit Exponent (-2)
    0x65, 0x11,        //     Unit (System: SI Linear, Length: Centimeter)
    0x09, 0x30,        //     Usage (X)
    0x35, 0x00,        //     Physical Minimum (0)
    0x46, 0x06, 0x04,  //     Physical Maximum (1030)
    0x95, 0x01,        //     Report Count (1)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x46, 0x49, 0x02,  //     Physical Maximum (585)
    0x26, 0xA4, 0x06,  //     Logical Maximum (1700)
    0x09, 0x31,        //     Usage (Y)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              //   End Collection
    0x05, 0x0D,        //   Usage Page (Digitizer)
    0x09, 0x22,        //   Usage (Finger)
    0xA1, 0x02,        //   Collection (Logical)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0x01,        //     Logical Maximum (1)
    0x09, 0x47,        //     Usage (0x47)
    0x09, 0x42,        //     Usage (Tip Switch)
    0x95, 0x02,        //     Report Count (2)
    0x75, 0x01,        //     Report Size (1)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x01,        //     Report Count (1)
    0x75, 0x03,        //     Report Size (3)
    0x25, 0x05,        //     Logical Maximum (5)
    0x09, 0x51,        //     Usage (0x51)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x75, 0x01,        //     Report Size (1)
    0x95, 0x03,        //     Report Count (3)
    0x81, 0x03,        //     Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
    0x15, 0x00,        //     Logical Minimum (0)
    0x26, 0x54, 0x0B,  //     Logical Maximum (2900)
    0x75, 0x10,        //     Report Size (16)
    0x55, 0x0E,        //     Unit Exponent (-2)
    0x65, 0x11,        //     Unit (System: SI Linear, Length: Centimeter)
    0x09, 0x30,        //     Usage (X)
    0x35, 0x00,        //     Physical Minimum (0)
    0x46, 0x06, 0x04,  //     Physical Maximum (1030)
    0x95, 0x01,        //     Report Count (1)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x46, 0x49, 0x02,  //     Physical Maximum (585)
    0x26, 0xA4, 0x06,  //     Logical Maximum (1700)
    0x09, 0x31,        //     Usage (Y)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              //   End Collection
    0x05, 0x0D,        //   Usage Page (Digitizer)
    0x09, 0x22,        //   Usage (Finger)
    0xA1, 0x02,        //   Collection (Logical)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0x01,        //     Logical Maximum (1)
    0x09, 0x47,        //     Usage (0x47)
    0x09, 0x42,        //     Usage (Tip Switch)
    0x95, 0x02,        //     Report Count (2)
    0x75, 0x01,        //     Report Size (1)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x01,        //     Report Count (1)
    0x75, 0x03,        //     Report Size (3)
    0x25, 0x05,        //     Logical Maximum (5)
    0x09, 0x51,        //     Usage (0x51)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x75, 0x01,        //     Report Size (1)
    0x95, 0x03,        //     Report Count (3)
    0x81, 0x03,        //     Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
    0x15, 0x00,        //     Logical Minimum (0)
    0x26, 0x54, 0x0B,  //     Logical Maximum (2900)
    0x75, 0x10,        //     Report Size (16)
    0x55, 0x0E,        //     Unit Exponent (-2)
    0x65, 0x11,        //     Unit (System: SI Linear, Length: Centimeter)
    0x09, 0x30,        //     Usage (X)
    0x35, 0x00,        //     Physical Minimum (0)
    0x46, 0x06, 0x04,  //     Physical Maximum (1030)
    0x95, 0x01,        //     Report Count (1)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x46, 0x49, 0x02,  //     Physical Maximum (585)
    0x26, 0xA4, 0x06,  //     Logical Maximum (1700)
    0x09, 0x31,        //     Usage (Y)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              //   End Collection
    0x05, 0x0D,        //   Usage Page (Digitizer)
    0x55, 0x0C,        //   Unit Exponent (-4)
    0x66, 0x01, 0x10,  //   Unit (System: SI Linear, Time: Seconds)
    0x47, 0xFF, 0xFF, 0x00, 0x00,  //   Physical Maximum (65534)
    0x27, 0xFF, 0xFF, 0x00, 0x00,  //   Logical Maximum (65534)
    0x75, 0x10,        //   Report Size (16)
    0x95, 0x01,        //   Report Count (1)
    0x09, 0x56,        //   Usage (0x56)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0x54,        //   Usage (0x54)
    0x25, 0x7F,        //   Logical Maximum (127)
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x08,        //   Report Size (8)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x09,        //   Usage Page (Button)
    0x09, 0x01,        //   Usage (0x01)
    0x09, 0x02,        //   Usage (0x02)
    0x09, 0x03,        //   Usage (0x03)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x03,        //   Report Count (3)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x05,        //   Report Count (5)
    0x81, 0x03,        //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x0D,        //   Usage Page (Digitizer)
    0x85, RPT_ID_MAXCNT,//   Report ID
    0x09, 0x55,        //   Usage (0x55)
    0x09, 0x59,        //   Usage (0x59)
    0x75, 0x04,        //   Report Size (4)
    0x95, 0x02,        //   Report Count (2)
    0x25, 0x0F,        //   Logical Maximum (15)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x06, 0x00, 0xFF,  //   Usage Page (Vendor Defined 0xFF00)
    0x85, RPT_ID_PTPHQA, //   Report ID
    0x09, 0xC5,        //   Usage (0xC5)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x75, 0x08,        //   Report Size (8)
    0x96, 0x00, 0x01,  //   Report Count (256)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,              // End Collection
    #endif //(HID_RPT_PTP)
};

#if (HID_RPT_PTP)
/// HID PTP HQA description, host read feature
/// https://learn.microsoft.com/zh-cn/windows-hardware/design/component-guidelines/windows-precision-touchpad-required-hid-top-level-collections
const uint8_t ptphqa_blob[] = 
{
    0xFC, 0x28, 0xFE, 0x84, 0x40, 0xCB, 0x9A, 0x87, 0x0D, 0xBE, 0x57, 0x3C, 0xB6, 0x70, 0x09, 0x88,
    0x07, 0x97, 0x2D, 0x2B, 0xE3, 0x38, 0x34, 0xB6, 0x6C, 0xED, 0xB0, 0xF7, 0xE5, 0x9C, 0xF6, 0xC2,
    0x2E, 0x84, 0x1B, 0xE8, 0xB4, 0x51, 0x78, 0x43, 0x1F, 0x28, 0x4B, 0x7C, 0x2D, 0x53, 0xAF, 0xFC,
    0x47, 0x70, 0x1B, 0x59, 0x6F, 0x74, 0x43, 0xC4, 0xF3, 0x47, 0x18, 0x53, 0x1A, 0xA2, 0xA1, 0x71,
    0xC7, 0x95, 0x0E, 0x31, 0x55, 0x21, 0xD3, 0xB5, 0x1E, 0xE9, 0x0C, 0xBA, 0xEC, 0xB8, 0x89, 0x19,
    0x3E, 0xB3, 0xAF, 0x75, 0x81, 0x9D, 0x53, 0xB9, 0x41, 0x57, 0xF4, 0x6D, 0x39, 0x25, 0x29, 0x7C,
    0x87, 0xD9, 0xB4, 0x98, 0x45, 0x7D, 0xA7, 0x26, 0x9C, 0x65, 0x3B, 0x85, 0x68, 0x89, 0xD7, 0x3B,
    0xBD, 0xFF, 0x14, 0x67, 0xF2, 0x2B, 0xF0, 0x2A, 0x41, 0x54, 0xF0, 0xFD, 0x2C, 0x66, 0x7C, 0xF8,
    0xC0, 0x8F, 0x33, 0x13, 0x03, 0xF1, 0xD3, 0xC1, 0x0B, 0x89, 0xD9, 0x1B, 0x62, 0xCD, 0x51, 0xB7,
    0x80, 0xB8, 0xAF, 0x3A, 0x10, 0xC1, 0x8A, 0x5B, 0xE8, 0x8A, 0x56, 0xF0, 0x8C, 0xAA, 0xFA, 0x35,
    0xE9, 0x42, 0xC4, 0xD8, 0x55, 0xC3, 0x38, 0xCC, 0x2B, 0x53, 0x5C, 0x69, 0x52, 0xD5, 0xC8, 0x73,
    0x02, 0x38, 0x7C, 0x73, 0xB6, 0x41, 0xE7, 0xFF, 0x05, 0xD8, 0x2B, 0x79, 0x9A, 0xE2, 0x34, 0x60,
    0x8F, 0xA3, 0x32, 0x1F, 0x09, 0x78, 0x62, 0xBC, 0x80, 0xE3, 0x0F, 0xBD, 0x65, 0x20, 0x08, 0x13,
    0xC1, 0xE2, 0xEE, 0x53, 0x2D, 0x86, 0x7E, 0xA7, 0x5A, 0xC5, 0xD3, 0x7D, 0x98, 0xBE, 0x31, 0x48,
    0x1F, 0xFB, 0xDA, 0xAF, 0xA2, 0xA8, 0x6A, 0x89, 0xD6, 0xBF, 0xF2, 0xD3, 0x32, 0x2A, 0x9A, 0xE4,
    0xCF, 0x17, 0xB7, 0xB8, 0xF4, 0xE1, 0x33, 0x08, 0x24, 0x8B, 0xC4, 0x43, 0xA5, 0xE5, 0x24, 0xC2
};
#endif //(HID_RPT_PTP)


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
const uint8_t *hid_get_ptphqa_blob(uint16_t *plen)
{
    *plen = sizeof(ptphqa_blob);
    return ptphqa_blob;
}
#endif //(HID_RPT_PTP)
