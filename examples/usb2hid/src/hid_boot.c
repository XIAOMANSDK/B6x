#include "usbd.h"
#include "usbd_hid.h"
#include "keys.h"
#include "drvs.h"

#if (DEMO_HID_BOOT)

#define USBD_BCD                  USB_1_1 // Version
#define USBD_VID                  0xFFFF  // Vendor ID
#define USBD_PID                  0xFFF0  // Product ID
#define USBD_MAX_POWER            100     // unit in mA
#define USBD_LANGID_STRING        0x0409  // English(US)

#define ENB_KEYBD                 0
#define ENB_MOUSE                 1

#if ((ENB_KEYBD > 1) || (ENB_MOUSE > 1) || (ENB_KEYBD+ENB_MOUSE == 0))
#error "The Count of HID Interface be 1 or 2."
#endif


/*
 * Descriptor
 ****************************************************************************
 */

/*!< count of hid interface descriptor */
#define USB_HID_INTF_CNT          (ENB_KEYBD + ENB_MOUSE)
#define USB_HID_INTF_END          (USB_HID_INTF_CNT - 1)

/*!< config descriptor size (only in endpoint) */
#define USB_HID_CONFIG_SIZE       (9+(18+7)*USB_HID_INTF_CNT)

#if (ENB_KEYBD)
/*!< keyboard interface config */
#define KEYBD_INTF_NUM            0
#define KEYBD_IN_EP               0x81
#define KEYBD_IN_EP_SIZE          8
//INTERVAL:1, 2, 4, 8, 16, ... 2**n
#define KEYBD_IN_EP_INTERVAL      8
#define KEYBD_REPORT_DESC_SIZE    sizeof(hid_keybd_report_desc)

/*!< keyboard report descriptor */
static const uint8_t hid_keybd_report_desc[] = {
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
#endif

#if (ENB_MOUSE)
/*!< mouse interface config */
#define MOUSE_INTF_NUM            (0 + ENB_KEYBD)
#define MOUSE_IN_EP               (0x81 + ENB_KEYBD)
#define MOUSE_IN_EP_SIZE          4
//INTERVAL:1, 2, 4, 8, 16, ... 2**n
#define MOUSE_IN_EP_INTERVAL      1
#define MOUSE_REPORT_DESC_SIZE    sizeof(hid_mouse_report_desc)

/*!< mouse report descriptor */
static const uint8_t hid_mouse_report_desc[] = {
    0x05, 0x01, // Usage Page (Generic Desktop Ctrls)
    0x09, 0x02, // Usage (Mouse)
    0xA1, 0x01, // Collection (Application)
    0x09, 0x01, //   Usage (Pointer)
    0xA1, 0x00, //   Collection (Physical)
    0x05, 0x09, //     Usage Page (Button)
    0x19, 0x01, //     Usage Minimum (0x01)
    0x29, 0x03, //     Usage Maximum (0x03)
    0x15, 0x00, //     Logical Minimum (0)
    0x25, 0x01, //     Logical Maximum (1)
    0x95, 0x03, //     Report Count (3)
    0x75, 0x01, //     Report Size (1)
    0x81, 0x02, //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x01, //     Report Count (1)
    0x75, 0x05, //     Report Size (5)
    0x81, 0x01, //     Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x01, //     Usage Page (Generic Desktop Ctrls)
    0x09, 0x30, //     Usage (X)
    0x09, 0x31, //     Usage (Y)
    0x09, 0x38, //     Usage (Wheel)
    0x15, 0x81, //     Logical Minimum (-127)
    0x25, 0x7F, //     Logical Maximum (127)
    0x75, 0x08, //     Report Size (8)
    0x95, 0x03, //     Report Count (3)
    0x81, 0x06, //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,       //   End Collection
    0xC0,       // End Collection
};

#if (COPY_MOUSE)
static const uint8_t hid_mouse_report_desc[] = {
    0x05, 0x01, // Usage Page (Generic Desktop Ctrls)
    0x09, 0x02, // Usage (Mouse)
    0xA1, 0x01, // Collection (Application)
    0x09, 0x01, //   Usage (Pointer)
    0xA1, 0x00, //   Collection (Physical)
    0x05, 0x09, //     Usage Page (Button)
    0x19, 0x01, //     Usage Minimum (0x01)
    0x29, 0x03, //     Usage Maximum (0x03)
    0x15, 0x00, //     Logical Minimum (0)
    0x25, 0x01, //     Logical Maximum (1)
    0x75, 0x01, //     Report Size (1)
    0x95, 0x03, //     Report Count (3)
    0x81, 0x02, //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x75, 0x05, //     Report Size (5)
    0x95, 0x01, //     Report Count (1)
    0x81, 0x01, //     Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x01, //     Usage Page (Generic Desktop Ctrls)
    0x09, 0x30, //     Usage (X)
    0x09, 0x31, //     Usage (Y)
    0x09, 0x38, //     Usage (Wheel)
    0x15, 0x81, //     Logical Minimum (-127)
    0x25, 0x7F, //     Logical Maximum (127)
    0x75, 0x08, //     Report Size (8)
    0x95, 0x03, //     Report Count (3)
    0x81, 0x06, //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,       //   End Collection
    0xC0,       // End Collection
};

static const uint8_t hid_descriptor[] = {
    0x12, 0x01, 0x10, 0x01, 0x00, 0x00, 0x00, 0x08, 0x3A, 0x09, 0x12, 0x25, 0x00, 0x01, 0x01, 0x02, 0x00, 0x01,
    0x09, 0x02, 0x22, 0x00, 0x01, 0x01, 0x00, 0xE0, 0x32, 0x09, 0x04, 0x00, 0x00, 0x01, 0x03, 0x01, 0x02, 0x00, 0x09, 0x21, 0x11, 0x01, 0x00, 0x01, 0x22, 0x34, 0x00, 0x07, 0x05, 0x81, 0x03, 0x04, 0x00, 0x0A,
    0x04, 0x03, 0x09, 0x04,
    0x0E, 0x03, 0x50, 0x00, 0x69, 0x00, 0x78, 0x00, 0x41, 0x00, 0x72, 0x00, 0x74, 0x00,
    0x24, 0x03, 0x55, 0x00, 0x53, 0x00, 0x42, 0x00, 0x20, 0x00, 0x4F, 0x00, 0x70, 0x00, 0x74, 0x00, 0x69, 0x00, 0x63, 0x00, 0x61, 0x00, 0x6C, 0x00, 0x20, 0x00, 0x4D, 0x00, 0x6F, 0x00, 0x75, 0x00, 0x73, 0x00, 0x65, 0x00,
    /* Descriptor - EOF */
    0x00
};
#endif

#endif

/*!< hid device descriptor */
static const uint8_t hid_descriptor[] = {
    /* Descriptor - Device (Size:18) */
    USB_DEVICE_DESCRIPTOR_INIT(USBD_BCD, 0x00, 0x00, 0x00, USBD_VID, USBD_PID, 0x0002, 0x01),

    /* Descriptor - Configuration (Total Size:9+Intf_Size) */
    USB_CONFIG_DESCRIPTOR_INIT(USB_HID_CONFIG_SIZE, USB_HID_INTF_CNT, 
            0x01, USB_CONFIG_BUS_POWERED | USB_CONFIG_REMOTE_WAKEUP, USBD_MAX_POWER),

    #if (ENB_KEYBD)
    /* Descriptor - Keyboard Interface (Size:18+7*1) */
    HID_INTERFACE_INIT(KEYBD_INTF_NUM, 1, HID_SUBCLASS_BOOTIF, HID_PROTOCOL_KEYBOARD, 0, KEYBD_REPORT_DESC_SIZE),
    HID_ENDPOINT_DESC(KEYBD_IN_EP, KEYBD_IN_EP_SIZE, KEYBD_IN_EP_INTERVAL),
    #endif
    
    #if (ENB_MOUSE)
    /* Descriptor - Mouse Interface (Size:18+7*1) */
    HID_INTERFACE_INIT(MOUSE_INTF_NUM, 1, HID_SUBCLASS_BOOTIF, HID_PROTOCOL_MOUSE, 0, MOUSE_REPORT_DESC_SIZE),
    HID_ENDPOINT_DESC(MOUSE_IN_EP, MOUSE_IN_EP_SIZE, MOUSE_IN_EP_INTERVAL),
    #endif
    
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
    WCHAR('2'),
    WCHAR('.'),
    WCHAR('0'),
    WCHAR(' '),
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
//    WCHAR('7'),
    
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
    #if (ENB_KEYBD)
    HID_INTF_T(KEYBD_INTF_NUM, KEYBD_IN_EP, hid_keybd_report_desc),
    #endif
    #if (ENB_MOUSE)
    HID_INTF_T(MOUSE_INTF_NUM, MOUSE_IN_EP, hid_mouse_report_desc),
    #endif
};

/*!< table of endpoints */
static const usbd_ep_t endpoint_tab[] = {
    #if (ENB_KEYBD)
    USBD_EP_T(KEYBD_IN_EP, USB_EP_TYPE_INTERRUPT, KEYBD_IN_EP_SIZE, &usbd_hid_ep_in_handler),
    #endif
    #if (ENB_MOUSE)
    USBD_EP_T(MOUSE_IN_EP, USB_EP_TYPE_INTERRUPT, MOUSE_IN_EP_SIZE, &usbd_hid_ep_in_handler),
    #endif
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

volatile bool suspend = false;

__USBIRQ void usbd_notify_handler(uint8_t event, void *arg)
{
    switch (event)
    {
        case USBD_EVENT_RESET:
            suspend = false;
            usbd_hid_reset();
            break;
        
        case USBD_EVENT_SUSPEND:
            suspend = true;
            break;
        
        case USBD_EVENT_RESUME:
        case USBD_EVENT_CLR_REMOTE_WAKEUP:
            suspend = false;
            break;

        default:
            break;
    }
}


/*
 * Test Functions
 ****************************************************************************
 */

uint8_t hid_keybd_send_report(uint8_t code)
{
    uint8_t ret = 0;
    
    #if (ENB_KEYBD)
    uint8_t kyebd_report[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; //A
    
    kyebd_report[2] = code;
    
    ret = usbd_hid_send_report(KEYBD_IN_EP, 8, kyebd_report);
    USB_LOG_RAW("keybd(k:%d,ret:%d)\r\n", code, ret);
    #endif
    
    return ret;
}

uint8_t hid_mouse_send_report(int8_t x)
{
    uint8_t ret = 0;
    
    #if (ENB_MOUSE)
    uint8_t mouse_report[4] = {0x00/*btns*/, 0/*x*/, 0/*y*/, 0/*wheel*/};
    
    mouse_report[1] = x;
    ret = usbd_hid_send_report(MOUSE_IN_EP, MOUSE_IN_EP_SIZE, mouse_report);
    USB_LOG_RAW("mouse(x:%d,ret:%d)\r\n", mouse_report[1],ret);
    #endif
    
    return ret;
}

#if (USE_KEYS)
void usbd_wakeup(void)
{
    if (suspend && usbd_resume(1))
    {
        btmr_delay(48000, 30);
        usbd_resume(0);
    }
}

void usbd_hid_leds(uint8_t leds)
{
    if (leds & 0x02/*CAPS_LOCK*/)
        GPIO_DAT_CLR(LED2);
    else
        GPIO_DAT_SET(LED2);
}

void usbdTest(void)
{
    // keys_scan to send report
    static uint32_t btn_lvl = BTNS;
    uint8_t ret = 0;
    uint32_t value = GPIO_PIN_GET() & BTNS;
    uint32_t chng = btn_lvl ^ value;
    btn_lvl = value;
    
    if (chng) {
        uint8_t code = 0;
        
        usbd_wakeup();
        
        if ((chng & BTN1) && ((value & BTN1) == 0)) {
            code = HID_KEY_A;
        }
        if ((chng & BTN2) && ((value & BTN2) == 0)) {
            code = HID_KEY_B;
        }
        if ((chng & BTN3) && ((value & BTN3) == 0)) {
            code = HID_KEY_C;
        }
        
        debug("keys(val:%X,chng:%X,code:%d)\r\n", btn_lvl, chng, code);
        ret = hid_keybd_send_report(code);
        if (ret != 0) {
            debug("keys Fail(sta:%d)\r\n", ret);
        }
        
        if (code) {
            GPIO_DAT_CLR(LED1);
            hid_mouse_send_report(100);
        }
        else {
            GPIO_DAT_SET(LED1);
            hid_mouse_send_report(-100);
        }
    }
}
#else
/// circle data
int8_t x_offset[] = {-1,-2,-4,-5,-6,-7,-8,-9,-8,-8,-9,-8,-7,-6,-5,-4,-2,-1, 1, 2, 4, 5, 6, 7, 8, 9, 8,8,9,8,7,6,5,4,2,1};
int8_t y_offset[] = { 8, 9, 8, 7, 6, 5, 4, 2, 1,-1,-2,-4,-5,-6,-7,-8,-9,-8,-8,-9,-8,-7,-6,-5,-4,-2,-1,1,2,4,5,6,7,8,9,8};
void usbdTest(void)
{
    // circle polling to send report
    static uint8_t keynum = 0;
    uint8_t ret;
    
    if (!usbd_is_configured())
        return;

    #if (ENB_KEYBD)
    /*!< keyboard test */
    {
        uint8_t kyebd_report[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; //A
        if ((keynum % 2) == 0)
            kyebd_report[2] = HID_KEY_A + keynum/2;
        
        ret = usbd_hid_send_report(KEYBD_IN_EP, 8, kyebd_report);
        USB_LOG_RAW("hid_keybd(key:%d,ret:%d)\r\n", keynum, ret);
    }
    #endif
    #if (ENB_MOUSE)
    /*!< mouse test */
    {
        uint8_t mouse_report[4] = {0x00/*btns*/, 0/*x*/, 0/*y*/, 0/*wheel*/};

        mouse_report[1] = (x_offset[keynum] * 2);
        mouse_report[2] = (y_offset[keynum] * 2);
        ret = usbd_hid_send_report(MOUSE_IN_EP, MOUSE_IN_EP_SIZE, mouse_report);
        USB_LOG_RAW("hid_mouse(x:%d,ret:%d)\r\n", mouse_report[1],ret);
    }
    #endif

    if (ret == USBD_OK) {
        keynum = (keynum + 1) % sizeof(x_offset);
        bootDelayUs(990);
    }
}

#endif

void usbdInit(void)
{
    suspend = false;
    // enable USB clk and iopad
    rcc_usb_en();
    usbd_init();
    usbd_register(hid_descriptor, hid_configuration);
    NVIC_EnableIRQ(USB_IRQn);
    
    for (uint8_t idx = 0; idx < USB_HID_INTF_CNT; idx++) {
        usbd_hid_init(idx, &hid_interface[idx]);
    }
    
    keys_init();
}

#endif // (DEMO_HID_BOOT)
