#if (CFG_USB)

#include "usbd.h"
#include "usbd_hid.h"
#include "regs.h"
#include "drvs.h"
#define USBD_BCD           USB_1_1 // Version
#define USBD_VID           0xFFFF  // Vendor ID
#define USBD_PID           0xFF00  // Product ID
#define USBD_MAX_POWER     100     // unit in mA
#define USBD_LANGID_STRING 0x0409  // English(US)

#if (DBG_USB)
#include "dbg.h"
#define DEBUG(format, ...) debug("<%s,%d>" format "\r\n", __MODULE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#define debugHex(dat, len)
#endif

/*
 * Descriptor
 ****************************************************************************
 */

/*!< count of hid interface descriptor */
#define USB_HID_INTF_CNT          2
#define USB_HID_INTF_END          (USB_HID_INTF_CNT - 1)

/*!< config descriptor size (in & out endpoints) */
#define USB_HID_CONFIG_SIZE       (9+(18+7+7)*USB_HID_INTF_CNT)

/*!< keyboard interface config */
#define KBD_INTF_NUM              0    /*!< interfaceNumber */
#ifndef KBD_IN_EP
#define KBD_IN_EP                 0x81 /*!< address */
#endif
#define KBD_IN_EP_SIZE            8    /*!< max packet length */
#define KBD_IN_EP_INTERVAL        4   /*!< polling time */
#define KBD_OUT_EP                0x01
#define KBD_OUT_EP_SIZE           1
#define KBD_OUT_EP_INTERVAL       4
#define KBD_REPORT_DESC_SIZE      sizeof(hid_kbd_report_desc)

/*!< custom-raw interface config */
#define RAW_INTF_NUM              1
#ifndef RAW_IN_EP
#define RAW_IN_EP                 0x82
#endif
#ifndef RAW_IN_EP_SIZE
#define RAW_IN_EP_SIZE            64
#endif
#define RAW_IN_EP_INTERVAL        1
#define RAW_OUT_EP                0x02
#define RAW_OUT_EP_SIZE           64
#define RAW_OUT_EP_INTERVAL       1
#define RAW_REPORT_DESC_SIZE      sizeof(hid_raw_report_desc)

/*!< Declaration of endpoint Handlers  */
void usbd_hid_kbd_out_handler(uint8_t ep);
void usbd_hid_raw_out_handler(uint8_t ep);

/*!< hid keyboard report descriptor */
static const uint8_t hid_kbd_report_desc[] = {
#if (0)
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x06,        // Usage (Keyboard)
    0xA1, 0x01,        // Collection (Application)
//    0x85, 0x01,        //   Report ID (1)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x19, 0xE0,        //   Usage Minimum (0xE0)
    0x29, 0xE7,        //   Usage Maximum (0xE7)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x08,        //   Report Count (8)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x08,        //   Report Size (8)
    0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x05,        //   Report Count (5)
    0x75, 0x01,        //   Report Size (1)
    0x05, 0x08,        //   Usage Page (LEDs)
    0x19, 0x01,        //   Usage Minimum (Num Lock)
    0x29, 0x05,        //   Usage Maximum (Kana)
    0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x03,        //   Report Size (3)
    0x91, 0x01,        //   Output (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x95, 0x06,        //   Report Count (6)
    0x75, 0x08,        //   Report Size (8)
    0x15, 0x28,        //   Logical Minimum (40)
    0x25, 0xFE,        //   Logical Maximum (-2)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x19, 0x28,        //   Usage Minimum (0x28)
    0x29, 0xFE,        //   Usage Maximum (0xFE)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
#if (0)
    0x05, 0x0C,        //   Usage Page (Consumer)
    0x09, 0x01,        //   Usage (Consumer Control)
    0xA1, 0x01,        //   Collection (Application)
    0x85, 0xF1,        //     Report ID (-15)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0x01,        //     Logical Maximum (1)
    0x75, 0x01,        //     Report Size (1)
    0x95, 0x18,        //     Report Count (24)
    0x09, 0xB5,        //     Usage (Scan Next Track)
    0x09, 0xB6,        //     Usage (Scan Previous Track)
    0x09, 0xB7,        //     Usage (Stop)
    0x09, 0xCD,        //     Usage (Play/Pause)
    0x09, 0xE2,        //     Usage (Mute)
    0x09, 0xE5,        //     Usage (Bass Boost)
    0x09, 0xE7,        //     Usage (Loudness)
    0x09, 0xE9,        //     Usage (Volume Increment)
    0x09, 0xEA,        //     Usage (Volume Decrement)
    0x0A, 0x52, 0x01,  //     Usage (Bass Increment)
    0x0A, 0x53, 0x01,  //     Usage (Bass Decrement)
    0x0A, 0x54, 0x01,  //     Usage (Treble Increment)
    0x0A, 0x55, 0x01,  //     Usage (Treble Decrement)
    0x0A, 0x83, 0x01,  //     Usage (AL Consumer Control Configuration)
    0x0A, 0x8A, 0x01,  //     Usage (AL Email Reader)
    0x0A, 0x92, 0x01,  //     Usage (AL Calculator)
    0x0A, 0x94, 0x01,  //     Usage (AL Local Machine Browser)
    0x0A, 0x21, 0x02,  //     Usage (AC Search)
    0x0A, 0x23, 0x02,  //     Usage (AC Home)
    0x0A, 0x24, 0x02,  //     Usage (AC Back)
    0x0A, 0x25, 0x02,  //     Usage (AC Forward)
    0x0A, 0x26, 0x02,  //     Usage (AC Stop)
    0x0A, 0x27, 0x02,  //     Usage (AC Refresh)
    0x0A, 0x2A, 0x02,  //     Usage (AC Bookmarks)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              //   End Collection
#endif
    0xC0,              // End Collection
#else
    0x05, 0x01, // USAGE_PAGE (Generic Desktop)
    0x09, 0x06, // USAGE (Keyboard)
    0xa1, 0x01, // COLLECTION (Application)
    0x05, 0x07, // USAGE_PAGE (Keyboard)
    0x19, 0xe0, // USAGE_MINIMUM (Keyboard LeftControl)
    0x29, 0xe7, // USAGE_MAXIMUM (Keyboard Right GUI)
    0x15, 0x00, // LOGICAL_MINIMUM (0)
    0x25, 0x01, // LOGICAL_MAXIMUM (1)
    0x75, 0x01, // REPORT_SIZE (1)
    0x95, 0x08, // REPORT_COUNT (8)
    0x81, 0x02, // INPUT (Data,Var,Abs)
    0x95, 0x01, // REPORT_COUNT (1)
    0x75, 0x08, // REPORT_SIZE (8)
    0x81, 0x03, // INPUT (Cnst,Var,Abs)
    0x95, 0x05, // REPORT_COUNT (5)
    0x75, 0x01, // REPORT_SIZE (1)
    0x05, 0x08, // USAGE_PAGE (LEDs)
    0x19, 0x01, // USAGE_MINIMUM (Num Lock)
    0x29, 0x05, // USAGE_MAXIMUM (Kana)
    0x91, 0x02, // OUTPUT (Data,Var,Abs)
    0x95, 0x01, // REPORT_COUNT (1)
    0x75, 0x03, // REPORT_SIZE (3)
    0x91, 0x03, // OUTPUT (Cnst,Var,Abs)
    0x95, 0x06, // REPORT_COUNT (6)
    0x75, 0x08, // REPORT_SIZE (8)
    0x15, 0x00, // LOGICAL_MINIMUM (0)
    0x25, 0xFF, // LOGICAL_MAXIMUM (255)
    0x05, 0x07, // USAGE_PAGE (Keyboard)
    0x19, 0x00, // USAGE_MINIMUM (Reserved (no event indicated))
    0x29, 0xFF, // USAGE_MAXIMUM (Keyboard Application)
    0x81, 0x00, // INPUT (Data,Ary,Abs)
    0xc0        // END_COLLECTION
#endif
};

/*!< hid custom-raw report descriptor */
static const uint8_t hid_raw_report_desc[] = {
    0x06, 0x00, 0xff, // USAGE_PAGE (Vendor Defined Page 1)
    0x09, 0x01,       // USAGE (Vendor Usage 1)
    0xa1, 0x01,       // COLLECTION (Application)
    0x09, 0x01,       // USAGE (Vendor Usage 1)
    0x15, 0x00,       // LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00, // LOGICAL_MAXIMUM (255)
    0x95, 0x40,       // REPORT_COUNT (64)
    0x75, 0x08,       // REPORT_SIZE (8)
    0x81, 0x02,       // INPUT (Data,Var,Abs)
    0x09, 0x01,       // USAGE (Vendor Usage 1)
    0x15, 0x00,       // LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00, // LOGICAL_MAXIMUM (255)
    0x95, 0x40,       // REPORT_COUNT (64)
    0x75, 0x08,       // REPORT_SIZE (8)
    0x91, 0x02,       // OUTPUT (Data,Var,Abs)
    0xC0              // END_COLLECTION
};

/*!< hid device descriptor */
static const uint8_t hid_descriptor[] = {
    /* Descriptor - Device (Size:18) */
    USB_DEVICE_DESCRIPTOR_INIT(USBD_BCD, 0x00, 0x00, 0x00, USBD_VID, USBD_PID, 0x0002, 0x01),
    
    /* Descriptor - Configuration (Total Size:9+Intf_Size) */
    USB_CONFIG_DESCRIPTOR_INIT(USB_HID_CONFIG_SIZE, USB_HID_INTF_CNT, 
            0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),

    /* Descriptor - Keyboard Interface (Size:18+7*2) */
    HID_INTERFACE_INIT(KBD_INTF_NUM, 2, HID_SUBCLASS_BOOTIF, HID_PROTOCOL_KEYBOARD, 0, KBD_REPORT_DESC_SIZE),
    HID_ENDPOINT_DESC(KBD_IN_EP, KBD_IN_EP_SIZE, KBD_IN_EP_INTERVAL),
    HID_ENDPOINT_DESC(KBD_OUT_EP, KBD_OUT_EP_SIZE, KBD_OUT_EP_INTERVAL),
    
    /* Descriptor - Custom-Raw Interface (Size:18+7*2) */
    HID_INTERFACE_INIT(RAW_INTF_NUM, 2, HID_SUBCLASS_BOOTIF, HID_PROTOCOL_NONE, 0, RAW_REPORT_DESC_SIZE),
    HID_ENDPOINT_DESC(RAW_IN_EP, RAW_IN_EP_SIZE, RAW_IN_EP_INTERVAL),
    HID_ENDPOINT_DESC(RAW_OUT_EP, RAW_OUT_EP_SIZE, RAW_OUT_EP_INTERVAL),
 
    /* Descriptor - String */
    // String0 - Language ID (Size:4)
    USB_LANGID_INIT(USBD_LANGID_STRING),
    
    // String1 - iManufacturer
    0x02,                       /* bLength */
    USB_DESC_TYPE_STRING,       /* bDescriptorType */
    
    // String2 - iProduct
    0x20,                       /* bLength */
    USB_DESC_TYPE_STRING,       /* bDescriptorType */
    WCHAR('U'),
    WCHAR('S'),
    WCHAR('B'),
    WCHAR(' '),
    WCHAR('R'),
    WCHAR('e'),
    WCHAR('c'),
    WCHAR('e'),
    WCHAR('i'),
    WCHAR('v'),
    WCHAR('e'),
    WCHAR(' '),
    WCHAR('M'),
    WCHAR('i'),
    WCHAR('c'),
    
    // String3 - iSerialNumber
    0x10,                       /* bLength */
    USB_DESC_TYPE_STRING,       /* bDescriptorType */
    WCHAR('6'),
    WCHAR('.'),
    WCHAR('2'),
    WCHAR('2'),
    WCHAR('.'),
    WCHAR('0'),
    WCHAR('8'),
    
    /* Descriptor - Device Qualifier (Size:10) */
    #if (USBD_BCD == USB_2_0)
    USB_QUALIFIER_INIT(0x01),
    #endif
    
    /* Descriptor - EOF */
    0x00
};


/*
 * Configuration
 ****************************************************************************
 */

/*!< table of hid interface */
static const hid_intf_t hid_interface[] = {
    HID_INTF_T(KBD_INTF_NUM, KBD_IN_EP, hid_kbd_report_desc),
    HID_INTF_T(RAW_INTF_NUM, RAW_IN_EP, hid_raw_report_desc),
};

/*!< table of endpoints */
static const usbd_ep_t endpoint_tab[] = {
    USBD_EP_T(KBD_IN_EP,  USB_EP_TYPE_INTERRUPT, KBD_IN_EP_SIZE,  &usbd_hid_ep_in_handler),
    USBD_EP_T(KBD_OUT_EP, USB_EP_TYPE_INTERRUPT, KBD_OUT_EP_SIZE, &usbd_hid_kbd_out_handler),

    USBD_EP_T(RAW_IN_EP,  USB_EP_TYPE_INTERRUPT, RAW_IN_EP_SIZE,  &usbd_hid_ep_in_handler),
    USBD_EP_T(RAW_OUT_EP, USB_EP_TYPE_INTERRUPT, RAW_OUT_EP_SIZE, &usbd_hid_raw_out_handler),
};

/*!< table of class */
static const usbd_class_t class_tab[] = {
    USBD_CLASS_T(0, USB_HID_INTF_END, &usbd_hid_class_handler),
};

/*!< USBD Configuration */
static const usbd_config_t hid_configuration[] = {
    USBD_CONFIG_T(1, USB_HID_INTF_CNT, class_tab, endpoint_tab)
};

/*
 * Handlers
 ****************************************************************************
 */

void usbd_hid_kbd_out_handler(uint8_t ep)
{
    uint8_t led_state;
    
    /*!< read the led data from host send */
    usbd_ep_read(ep, KBD_OUT_EP_SIZE, &led_state);
    DEBUG("led_state:%02X\r\n", led_state);
    
    /*!< here you can write the LED processing from the host */
    usbd_hid_leds(led_state);
}

void usbd_hid_raw_out_handler(uint8_t ep)
{
    uint8_t custom_data[RAW_OUT_EP_SIZE];
    
    /*!< read the data from host send */
    usbd_ep_read(RAW_OUT_EP, RAW_OUT_EP_SIZE, custom_data);

    /*!< you can use the data do some thing you like */
}

bool suspend;

__USBIRQ void usbd_notify_handler(uint8_t event, void *arg)
{
    DEBUG("evt:%d", event);
    switch (event)
    {
        case USBD_EVENT_RESET:
        {
            suspend = false;
            usbd_hid_reset();
        } break;

        case USBD_EVENT_SUSPEND:
        {
            suspend = true;
        } break;

        case USBD_EVENT_RESUME:
        case USBD_EVENT_CLR_REMOTE_WAKEUP:
        {
            suspend = false;
        } break;

        default:
            break;
    }
}

void usbd_wakeup(void)
{
    if (suspend && usbd_resume(1))
    {
        btmr_delay(48000, 30);
        usbd_resume(0);
    }
}

void usbdInit(void)
{
    suspend = false;
    // enable USB clk and iopad
    rcc_usb_en();
    // .DIG_USB_PU = 1, 1.87K
    // .DIG_USB_PU = 2, 1.32K
    // .DIG_USB_PU = 3, 1.27K
    SYSCFG->USB_CTRL.Word = 0x14;
//    CSC->CSC_OUTPUT[6].Word = 0;
//    CSC->CSC_OUTPUT[7].Word = 0;
//    CSC->CSC_INPUT[CSC_UART1_TXD].Word = 0;
//    CSC->CSC_INPUT[CSC_UART1_RXD].Word = 0;
    
    usbd_init();
    usbd_register(hid_descriptor, hid_configuration);

    NVIC_SetPriority(BLE_IRQn, 1);
    NVIC_EnableIRQ(USB_IRQn);
    
    for (uint8_t idx = 0; idx < USB_HID_INTF_CNT; idx++)
    {
        usbd_hid_init(idx, &hid_interface[idx]);
    }
}
#endif // (CFG_USB)
