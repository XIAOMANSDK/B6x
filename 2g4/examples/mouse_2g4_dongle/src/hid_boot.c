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
#define USB_HID_INTF_CNT (1)
#define USB_HID_INTF_END (USB_HID_INTF_CNT - 1)

/*!< config descriptor size (only in endpoint) */
#define USB_HID_CONFIG_SIZE (9 + (18 + 7) * USB_HID_INTF_CNT)

/*!< mouse interface config */
#define MOUSE_INTF_NUM (0)
#if (CFG_USE_RPT_ID)
#define MOUSE_IN_EP_SIZE 5
#else
#define MOUSE_IN_EP_SIZE 4
#endif
//1, 2, 4, 8, 16, ...
#define MOUSE_IN_EP_INTERVAL   1 // 1ms
#define MOUSE_REPORT_DESC_SIZE sizeof(hid_mouse_report_desc)

/*!< mouse report descriptor */
static const uint8_t hid_mouse_report_desc[] =
{
    0x05, 0x01, // Usage Page (Generic Desktop Ctrls)
    0x09, 0x02, // Usage (Mouse)
    0xA1, 0x01, // Collection (Application)
#if (CFG_USE_RPT_ID)
    0x85, 0x01, //   Report ID
#endif
    0x09, 0x01, //   Usage (Pointer)
    0xA1, 0x00, //   Collection (Physical)
    0x05, 0x09, //     Usage Page (Button)
    0x19, 0x01, //     Usage Minimum (0x01)
    0x29, 0x08, //     Usage Maximum (0x08)
    0x15, 0x00, //     Logical Minimum (0)
    0x25, 0x01, //     Logical Maximum (1)
    0x75, 0x01, //     Report Size (1)
    0x95, 0x08, //     Report Count (8)
    0x81, 0x02, //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x01, //     Usage Page (Generic Desktop Ctrls)

#if (XY_nB == 3)
    // XY 4B
    0x17, 0x02, 0x00, 0xFF, 0xFF, //     Logical Minimum (-65534)
    0x27, 0xFF, 0xFF, 0x00, 0x00, //     Logical Maximum (65535)
    0x75, 0x10, //     Report Size(16)
#elif (XY_nB == 2)
    // XY 2B
    0x16, 0x08, 0xFF,  //     Logical Minimum (-248)
    0x26, 0xFF, 0x00,  //     Logical Maximum (255)
    0x75, 0x10,       //     Report Size(16)
#else
    // XY 1B
    0x15, 0x81, //     Logical Minimum (-127)
    0x25, 0x7F, //     Logical Maximum(127)
    0x75, 0x08, //     Report Size(8)
#endif

    0x95, 0x02, //     Report Size(2)
    0x09, 0x30, //     Usage (X)
    0x09, 0x31, //     Usage (Y)
    0x81, 0x06, //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0x15, 0x81, //     Logical Minimum (-127)
    0x25, 0x7F, //     Logical Maximum (127)
    0x75, 0x08, //     Report Size (8)
    0x95, 0x01, //     Report Count (1)
    0x09, 0x38, //     Usage (Wheel)
    0x81, 0x06, //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,       //   End Collection
    0xC0,       // End Collection
};

/*!< hid device descriptor */
static const uint8_t hid_descriptor[] =
{
    /* Descriptor - Device (Size:18) */
    USB_DEVICE_DESCRIPTOR_INIT(USBD_BCD, 0x00, 0x00, 0x00, USBD_VID, USBD_PID, 0x0002/*USBD_BCD*/, 0x01),

    /* Descriptor - Configuration (Total Size:9+Intf_Size) */
    USB_CONFIG_DESCRIPTOR_INIT(USB_HID_CONFIG_SIZE, USB_HID_INTF_CNT, 0x01,
                               USB_CONFIG_BUS_POWERED | USB_CONFIG_REMOTE_WAKEUP, USBD_MAX_POWER),

    /* Descriptor - Mouse Interface (Size:18+7*1) */
    HID_INTERFACE_INIT(MOUSE_INTF_NUM, 1, HID_SUBCLASS_BOOTIF, HID_PROTOCOL_MOUSE, 0, MOUSE_REPORT_DESC_SIZE),
    HID_ENDPOINT_DESC(MOUSE_IN_EP, MOUSE_IN_EP_SIZE, MOUSE_IN_EP_INTERVAL),

    /* Descriptor - String */
    // String0 - Language ID (Size:4)
    USB_LANGID_INIT(USBD_LANGID_STRING),

    // String1 - iManufacturer
    0x02,                 /* bLength */
    USB_DESC_TYPE_STRING, /* bDescriptorType */

    // String2 - iProduct
    0x1A,                 /* bLength */
    USB_DESC_TYPE_STRING, /* bDescriptorType */
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
    WCHAR('r'),

    // String3 - iSerialNumber
//    0x02,                 /* bLength */
//    USB_DESC_TYPE_STRING, /* bDescriptorType */
//    0x0C,                 /* bLength */
//    USB_DESC_TYPE_STRING, /* bDescriptorType */
//    WCHAR('1'),
//    WCHAR('1'),
//    WCHAR('.'),
//    WCHAR('1'),
//    WCHAR('0'),

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
static const hid_intf_t hid_interface[] =
{
    HID_INTF_T(MOUSE_INTF_NUM, MOUSE_IN_EP, hid_mouse_report_desc),
};

/*!< table of endpoints */
static const usbd_ep_t endpoint_tab[] =
{
    USBD_EP_T(MOUSE_IN_EP, USB_EP_TYPE_INTERRUPT, MOUSE_IN_EP_SIZE, &usbd_hid_ep_in_handler),
};

/*!< table of class */
static const usbd_class_t class_tab[] =
{
    USBD_CLASS_T(0, USB_HID_INTF_END, &usbd_hid_class_handler),
};

/*!< USBD Configuration */
static const usbd_config_t hid_configuration[] =
{
    USBD_CONFIG_T(1, USB_HID_INTF_CNT, class_tab, endpoint_tab)
};

/*
 * Handlers
 ****************************************************************************
 */

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
    usbd_init();
    usbd_register(hid_descriptor, hid_configuration);

    NVIC_EnableIRQ(USB_IRQn);

    for (uint8_t idx = 0; idx < USB_HID_INTF_CNT; idx++)
    {
        usbd_hid_init(idx, &hid_interface[idx]);
    }
}
#endif // (CFG_USB)
