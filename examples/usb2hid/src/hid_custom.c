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
//INTERVAL:1, 2, 4, 8, 16, ... 2**n
#define KBD_OUT_EP_INTERVAL       8
#define KBD_REPORT_DESC_SIZE      sizeof(hid_kbd_report_desc)

/*!< custom-raw interface config */
#define RAW_INTF_NUM              1
#define RAW_IN_EP                 0x82
#define RAW_IN_EP_SIZE            64
#define RAW_IN_EP_INTERVAL        10
#define RAW_OUT_EP                0x02
#define RAW_OUT_EP_SIZE           64
//INTERVAL:1, 2, 4, 8, 16, ... 2**n
#define RAW_OUT_EP_INTERVAL       8
#define RAW_REPORT_DESC_SIZE      sizeof(hid_raw_report_desc)

/*!< Declaration of endpoint Handlers  */
void usbd_hid_kbd_out_handler(uint8_t ep);
void usbd_hid_raw_out_handler(uint8_t ep);

/*!< hid keyboard report descriptor */
static const uint8_t hid_kbd_report_desc[] = {
    0x05, 0x01, // Usage Page (Generic Desktop Ctrls)
    0x09, 0x06, // Usage (Keyboard)
    0xA1, 0x01, // Collection (Application)
    0x05, 0x07, //   Usage Page (Kbrd/Keypad)
    0x19, 0xE0, //   Usage Minimum (0xE0)
    0x29, 0xE7, //   Usage Maximum (0xE7)
    0x15, 0x00, //   Logical Minimum (0)
    0x25, 0x01, //   Logical Maximum (1)
    0x75, 0x01, //   Report Size (1)
    0x95, 0x08, //   Report Count (8)
    0x81, 0x02, //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x01, //   Report Count (1)
    0x75, 0x08, //   Report Size (8)
    0x81, 0x03, //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x05, //   Report Count (5)
    0x75, 0x01, //   Report Size (1)
    0x05, 0x08, //   Usage Page (LEDs)
    0x19, 0x01, //   Usage Minimum (Num Lock)
    0x29, 0x05, //   Usage Maximum (Kana)
    0x91, 0x02, //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x95, 0x01, //   Report Count (1)
    0x75, 0x03, //   Report Size (3)
    0x91, 0x03, //   Output (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x95, 0x06, //   Report Count (6)
    0x75, 0x08, //   Report Size (8)
    0x15, 0x00, //   Logical Minimum (0)
    0x25, 0xFF, //   Logical Maximum (-1)
    0x05, 0x07, //   Usage Page (Kbrd/Keypad)
    0x19, 0x00, //   Usage Minimum (0x00)
    0x29, 0x65, //   Usage Maximum (0x65)
    0x81, 0x00, //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,       // End Collection
};

/*!< hid custom-raw report descriptor */
static const uint8_t hid_raw_report_desc[] = {
    0x06, 0x00, 0xFF, // Usage Page (Vendor Defined 0xFF00)
    0x09, 0x01,       // Usage (0x01)
    0xA1, 0x01,       // Collection (Application)
    0x09, 0x01,       //   Usage (0x01)
    0x15, 0x00,       //   Logical Minimum (0)
    0x26, 0xFF, 0x00, //   Logical Maximum (255)
    0x95, 0x40,       //   Report Count (64)
    0x75, 0x08,       //   Report Size (8)
    0x81, 0x02,       //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0x01,       //   Usage (0x01)
    0x15, 0x00,       //   Logical Minimum (0)
    0x26, 0xFF, 0x00, //   Logical Maximum (255)
    0x95, 0x40,       //   Report Count (64)
    0x75, 0x08,       //   Report Size (8)
    0x91, 0x02,       //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,             // End Collection
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
    0x16,                       /* bLength */
    USB_DESC_TYPE_STRING,       /* bDescriptorType */
    WCHAR('U'),
    WCHAR('S'),
    WCHAR('B'),
    WCHAR(' '),
    WCHAR('R'),
    WCHAR('a'),
    WCHAR('w'),
    WCHAR('H'),
    WCHAR('I'),
    WCHAR('D'),
    
    // String3 - iSerialNumber
//    0x10,                       /* bLength */
//    USB_DESC_TYPE_STRING,       /* bDescriptorType */
//    WCHAR('6'),
//    WCHAR('.'),
//    WCHAR('2'),
//    WCHAR('2'),
//    WCHAR('.'),
//    WCHAR('0'),
//    WCHAR('8'),
    
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
    // enable USB clk and iopad
    rcc_usb_en();
    usbd_init();
    usbd_register(hid_descriptor, hid_configuration);
    NVIC_EnableIRQ(USB_IRQn);
    
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
