#include "usbd.h"
#include "usbd_hid.h"
#include "drvs.h"

#if (DEMO_HID_CUSTOM)

#define USBD_BCD                  USB_2_0 // Version
#define USBD_VID                  0xFFFF  // Vendor ID
#define USBD_PID                  0xFFFF  // Product ID
#define USBD_MAX_POWER            100     // unit in mA
#define USBD_LANGID_STRING        0x0409  // English(US)


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
#define KBD_IN_EP                 0x81 /*!< address */
#define KBD_IN_EP_SIZE            8    /*!< max packet length */
#define KBD_IN_EP_INTERVAL        10   /*!< polling time */
#define KBD_OUT_EP                0x01
#define KBD_OUT_EP_SIZE           1
#define KBD_OUT_EP_INTERVAL       10
#define KBD_REPORT_DESC_SIZE      sizeof(hid_kbd_report_desc)

/*!< custom-raw interface config */
#define RAW_INTF_NUM              1
#define RAW_IN_EP                 0x82
#define RAW_IN_EP_SIZE            64
#define RAW_IN_EP_INTERVAL        10
#define RAW_OUT_EP                0x02
#define RAW_OUT_EP_SIZE           64
#define RAW_OUT_EP_INTERVAL       10
#define RAW_REPORT_DESC_SIZE      sizeof(hid_raw_report_desc)

/*!< Declaration of endpoint Handlers  */
void usbd_hid_kbd_out_handler(uint8_t ep);
void usbd_hid_raw_out_handler(uint8_t ep);

/*!< hid keyboard report descriptor */
static const uint8_t hid_kbd_report_desc[] = {
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
    0x29, 0x65, // USAGE_MAXIMUM (Keyboard Application)
    0x81, 0x00, // INPUT (Data,Ary,Abs)
    0xc0        // END_COLLECTION
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
    0x14,                       /* bLength */
    USB_DESC_TYPE_STRING,       /* bDescriptorType */
    WCHAR('H'),                 /* wcChar0 */
    WCHAR('Y'),                 /* wcChar1 */
    WCHAR('I'),                 /* wcChar2 */
    WCHAR('C'),                 /* wcChar3 */
    WCHAR(' '),                 /* wcChar4 */
    WCHAR('I'),                 /* wcChar5 */
    WCHAR('n'),                 /* wcChar6 */
    WCHAR('c'),                 /* wcChar7 */
    WCHAR('.'),                 /* wcChar8 */
    // String2 - iProduct
    0x1C,                       /* bLength */
    USB_DESC_TYPE_STRING,       /* bDescriptorType */
    WCHAR('H'),                 /* wcChar0 */
    WCHAR('Y'),                 /* wcChar1 */
    WCHAR('-'),                 /* wcChar2 */
    WCHAR('U'),                 /* wcChar3 */
    WCHAR('S'),                 /* wcChar4 */
    WCHAR('B'),                 /* wcChar5 */
    WCHAR(' '),                 /* wcChar6 */
    WCHAR('R'),                 /* wcChar7 */
    WCHAR('a'),                 /* wcChar8 */
    WCHAR('w'),                 /* wcChar9 */
    WCHAR('H'),                 /* wcChar10 */
    WCHAR('I'),                 /* wcChar11 */
    WCHAR('D'),                 /* wcChar12 */
    // String3 - iSerialNumber
    0x10,                       /* bLength */
    USB_DESC_TYPE_STRING,       /* bDescriptorType */
    WCHAR('6'),                 /* wcChar0 */
    WCHAR('.'),                 /* wcChar1 */
    WCHAR('2'),                 /* wcChar2 */
    WCHAR('2'),                 /* wcChar3 */
    WCHAR('.'),                 /* wcChar4 */
    WCHAR('0'),                 /* wcChar5 */
    WCHAR('8'),                 /* wcChar6 */
    
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
    USB_LOG_RAW("led_state:%02X\r\n", led_state);
    
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

__USBIRQ void usbd_notify_handler(uint8_t event, void *arg)
{
    switch (event) {
        case USBD_EVENT_RESET:
            usbd_hid_reset();
            break;
        case USBD_EVENT_RESUME:
            break;
        case USBD_EVENT_SUSPEND:
            break;

        default:
            break;
    }
}


/*
 * Test Functions
 ****************************************************************************
 */

void usbdInit(void)
{
    usbd_init();
    usbd_register(hid_descriptor, hid_configuration);
    
    for (uint8_t idx = 0; idx < USB_HID_INTF_CNT; idx++) {
        usbd_hid_init(idx, &hid_interface[idx]);
    }
}

void usbdTest(void)
{
    // circle polling to send report
    static uint8_t keynum = 0;
    
    if (!usbd_is_configured())
        return;
    
    /*!< keyboard test */
    {
        uint8_t sendbuffer1[KBD_IN_EP_SIZE] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; //A
    
        sendbuffer1[2] = HID_KEY_A + keynum;
        usbd_hid_send_report(KBD_IN_EP, KBD_IN_EP_SIZE, sendbuffer1);
        /*!< delay 10ms */
        bootDelayMs(10);
        /*!< send button up */
        sendbuffer1[2] = 0;
        usbd_hid_send_report(KBD_IN_EP, KBD_IN_EP_SIZE, sendbuffer1);
    }
    
    /*!< delay 100ms the custom test */
    {
        bootDelayMs(100);
        /*!< custom test */
        uint8_t sendbuffer2[RAW_IN_EP_SIZE];
        
        memset(sendbuffer2, keynum, RAW_IN_EP_SIZE);
        usbd_hid_send_report(RAW_IN_EP, RAW_IN_EP_SIZE, sendbuffer2);
    }
    
    if (++keynum > 94) keynum = 0;
}

#endif // (DEMO_HID_CUSTOM)
