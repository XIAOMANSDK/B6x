/**
 ****************************************************************************************
 *
 * @file hid_boot.c
 *
 * @brief USB HID Boot device implementation (keyboard + mouse)
 *
 * @details
 * DEMO_HID_BOOT example:
 * - Device init: enable USB clock, init USB controller, register descriptors
 * - Enumeration: host reads device/config descriptors, completes enumeration
 * - Data transfer: periodically send keyboard and mouse reports via interrupt endpoints
 * - Power management: handle suspend/resume events, support remote wakeup
 * - User interaction: generate test data via button scan or auto-timer loop
 *
 * USB HID device based on USB Human Interface Device class specification:
 * - Descriptor definition: device, config, interface, endpoint, and report descriptors
 * - Interrupt transfer: low-latency data transfer via interrupt endpoints
 * - Report mechanism: predefined report formats for device-host data exchange
 * - Protocol handling: USB core handles standard requests, HID class handles HID-specific
 *
 ****************************************************************************************
 */

#include "usbd.h"
#include "usbd_hid.h"
#include "keys.h"
#include "drvs.h"
#include "dbg.h"

#if (DEMO_HID_BOOT)

/*
 * DEFINES - USB Device Configuration
 ****************************************************************************************
 */

#define USBD_BCD                  USB_1_1     ///< USB specification version
#define USBD_VID                  0xFFFF      ///< Vendor ID
#define USBD_PID                  0xFFF0      ///< Product ID
#define USBD_MAX_POWER            100         ///< Max power (mA)
#define USBD_LANGID_STRING        0x0409      ///< English (US)

/*
 * DEFINES - HID Interface Configuration
 ****************************************************************************************
 */

/// Enable keyboard interface (0=disable, 1=enable)
#define ENB_KEYBD                 0

/// Enable mouse interface (0=disable, 1=enable)
#define ENB_MOUSE                 1

#if ((ENB_KEYBD > 1) || (ENB_MOUSE > 1) || (ENB_KEYBD + ENB_MOUSE == 0))
#error "The Count of HID Interface be 1 or 2."
#endif

/*
 * DEFINES - Descriptor Sizes
 ****************************************************************************************
 */

/// Total HID interface count
#define USB_HID_INTF_CNT          (ENB_KEYBD + ENB_MOUSE)

/// Last interface number
#define USB_HID_INTF_END          (USB_HID_INTF_CNT - 1)

/// Configuration descriptor total size
#define USB_HID_CONFIG_SIZE       (9 + (18 + 7) * USB_HID_INTF_CNT)

/*
 * DEFINES - Keyboard Endpoint Configuration
 ****************************************************************************************
 */

#if (ENB_KEYBD)
#define KEYBD_INTF_NUM            0           ///< Keyboard interface number
#define KEYBD_IN_EP               0x81        ///< Keyboard IN endpoint address
#define KEYBD_IN_EP_SIZE          8           ///< Keyboard endpoint max packet size
/// Endpoint polling interval: 1, 2, 4, 8, 16, ... 2^n
#define KEYBD_IN_EP_INTERVAL      8
#define KEYBD_REPORT_DESC_SIZE    sizeof(hid_keybd_report_desc)

/// Keyboard HID report descriptor
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
    0x81, 0x02, //   Input (Data,Var,Abs)
    0x95, 0x01, //   Report Count (1)
    0x75, 0x08, //   Report Size (8)
    0x81, 0x03, //   Input (Const,Var,Abs)
    0x95, 0x05, //   Report Count (5)
    0x75, 0x01, //   Report Size (1)
    0x05, 0x08, //   Usage Page (LEDs)
    0x19, 0x01, //   Usage Minimum (Num Lock)
    0x29, 0x05, //   Usage Maximum (Kana)
    0x91, 0x02, //   Output (Data,Var,Abs)
    0x95, 0x01, //   Report Count (1)
    0x75, 0x03, //   Report Size (3)
    0x91, 0x03, //   Output (Const,Var,Abs)
    0x95, 0x06, //   Report Count (6)
    0x75, 0x08, //   Report Size (8)
    0x15, 0x00, //   Logical Minimum (0)
    0x25, 0xFF, //   Logical Maximum (-1)
    0x05, 0x07, //   Usage Page (Kbrd/Keypad)
    0x19, 0x00, //   Usage Minimum (0x00)
    0x29, 0x65, //   Usage Maximum (0x65)
    0x81, 0x00, //   Input (Data,Array,Abs)
    0xC0,       // End Collection
};
#endif

/*
 * DEFINES - Mouse Endpoint Configuration
 ****************************************************************************************
 */

#if (ENB_MOUSE)
#define MOUSE_INTF_NUM            (0 + ENB_KEYBD)         ///< Mouse interface number
#define MOUSE_IN_EP               (0x81 + ENB_KEYBD)      ///< Mouse IN endpoint address
#define MOUSE_IN_EP_SIZE          4                        ///< Mouse endpoint max packet size
/// Endpoint polling interval: 1, 2, 4, 8, 16, ... 2^n
#define MOUSE_IN_EP_INTERVAL      1
#define MOUSE_REPORT_DESC_SIZE    sizeof(hid_mouse_report_desc)

/// Mouse HID report descriptor
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
    0x81, 0x02, //     Input (Data,Var,Abs)
    0x95, 0x01, //     Report Count (1)
    0x75, 0x05, //     Report Size (5)
    0x81, 0x01, //     Input (Const,Array,Abs)
    0x05, 0x01, //     Usage Page (Generic Desktop Ctrls)
    0x09, 0x30, //     Usage (X)
    0x09, 0x31, //     Usage (Y)
    0x09, 0x38, //     Usage (Wheel)
    0x15, 0x81, //     Logical Minimum (-127)
    0x25, 0x7F, //     Logical Maximum (127)
    0x75, 0x08, //     Report Size (8)
    0x95, 0x03, //     Report Count (3)
    0x81, 0x06, //     Input (Data,Var,Rel)
    0xC0,       //   End Collection
    0xC0,       // End Collection
};
#endif

/*
 * LOCAL DATA - USB Descriptors
 ****************************************************************************************
 */

/// HID device descriptor
static const uint8_t hid_descriptor[] = {
    /* Device descriptor (18 bytes) */
    USB_DEVICE_DESCRIPTOR_INIT(USBD_BCD, 0x00, 0x00, 0x00, USBD_VID, USBD_PID, 0x0002, 0x01),

    /* Configuration descriptor */
    USB_CONFIG_DESCRIPTOR_INIT(USB_HID_CONFIG_SIZE, USB_HID_INTF_CNT,
            0x01, USB_CONFIG_BUS_POWERED | USB_CONFIG_REMOTE_WAKEUP, USBD_MAX_POWER),

#if (ENB_KEYBD)
    /* Keyboard interface (18+7 bytes) */
    HID_INTERFACE_INIT(KEYBD_INTF_NUM, 1, HID_SUBCLASS_BOOTIF, HID_PROTOCOL_KEYBOARD, 0, KEYBD_REPORT_DESC_SIZE),
    HID_ENDPOINT_DESC(KEYBD_IN_EP, KEYBD_IN_EP_SIZE, KEYBD_IN_EP_INTERVAL),
#endif

#if (ENB_MOUSE)
    /* Mouse interface (18+7 bytes) */
    HID_INTERFACE_INIT(MOUSE_INTF_NUM, 1, HID_SUBCLASS_BOOTIF, HID_PROTOCOL_MOUSE, 0, MOUSE_REPORT_DESC_SIZE),
    HID_ENDPOINT_DESC(MOUSE_IN_EP, MOUSE_IN_EP_SIZE, MOUSE_IN_EP_INTERVAL),
#endif

    /* String 0 - Language ID */
    USB_LANGID_INIT(USBD_LANGID_STRING),

    /* String 1 - iManufacturer */
    0x02,
    USB_DESC_TYPE_STRING,

    /* String 2 - iProduct: "USB2.0 HID" */
    0x16,
    USB_DESC_TYPE_STRING,
    WCHAR('U'), WCHAR('S'), WCHAR('B'), WCHAR('2'),
    WCHAR('.'), WCHAR('0'), WCHAR(' '), WCHAR('H'),
    WCHAR('I'), WCHAR('D'),

    /* Device qualifier (USB 2.0 only) */
#if (USBD_BCD == USB_2_0)
    USB_QUALIFIER_INIT(0x01),
#endif

    /* End of descriptor */
    0x00
};

/*
 * LOCAL DATA - USB Configuration Tables
 ****************************************************************************************
 */

/// HID interface table
static const hid_intf_t hid_interface[] = {
#if (ENB_KEYBD)
    HID_INTF_T(KEYBD_INTF_NUM, KEYBD_IN_EP, hid_keybd_report_desc),
#endif
#if (ENB_MOUSE)
    HID_INTF_T(MOUSE_INTF_NUM, MOUSE_IN_EP, hid_mouse_report_desc),
#endif
};

/// Endpoint table
static const usbd_ep_t endpoint_tab[] = {
#if (ENB_KEYBD)
    USBD_EP_T(KEYBD_IN_EP, USB_EP_TYPE_INTERRUPT, KEYBD_IN_EP_SIZE, &usbd_hid_ep_in_handler),
#endif
#if (ENB_MOUSE)
    USBD_EP_T(MOUSE_IN_EP, USB_EP_TYPE_INTERRUPT, MOUSE_IN_EP_SIZE, &usbd_hid_ep_in_handler),
#endif
};

/// Class table
static const usbd_class_t class_tab[] = {
    USBD_CLASS_T(0, USB_HID_INTF_END, &usbd_hid_class_handler),
};

/// Device configuration
static const usbd_config_t hid_configuration[] = {
    USBD_CONFIG_T(1, USB_HID_INTF_CNT, class_tab, endpoint_tab)
};

/*
 * LOCAL DATA
 ****************************************************************************************
 */

/// USB suspend flag
static volatile bool suspend = false;

/*
 * CALLBACK FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief USB device event notification handler
 *
 * @param[in] event  Event type
 * @param[in] arg    Event argument (unused)
 ****************************************************************************************
 */
__USBIRQ void usbd_notify_handler(uint8_t event, void *arg)
{
    (void)arg;

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
 * REPORT FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Send keyboard report
 *
 * @param[in] code  Key code
 *
 * @return 0 on success, non-zero on failure
 ****************************************************************************************
 */
static uint8_t hid_keybd_send_report(uint8_t code)
{
    uint8_t ret = 0;

#if (ENB_KEYBD)
    uint8_t keybd_report[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    keybd_report[2] = code;

    ret = usbd_hid_send_report(KEYBD_IN_EP, 8, keybd_report);
    USB_LOG_RAW("keybd(k:%d,ret:%d)\r\n", code, ret);
#endif

    return ret;
}

/**
 ****************************************************************************************
 * @brief Send mouse report
 *
 * @param[in] x  X-axis movement
 *
 * @return 0 on success, non-zero on failure
 ****************************************************************************************
 */
static uint8_t hid_mouse_send_report(int8_t x)
{
    uint8_t ret = 0;

#if (ENB_MOUSE)
    uint8_t mouse_report[4] = { 0x00, 0x00, 0x00, 0x00 };

    mouse_report[1] = x;
    ret = usbd_hid_send_report(MOUSE_IN_EP, MOUSE_IN_EP_SIZE, mouse_report);
    USB_LOG_RAW("mouse(x:%d,ret:%d)\r\n", mouse_report[1], ret);
#endif

    return ret;
}

/*
 * TEST FUNCTIONS
 ****************************************************************************************
 */

#if (USE_KEYS)

/**
 ****************************************************************************************
 * @brief Trigger USB remote wakeup if suspended
 ****************************************************************************************
 */
static void usbd_wakeup(void)
{
    if (suspend && usbd_resume(1))
    {
        btmr_delay(48000, 30);
        usbd_resume(0);
    }
}

/**
 ****************************************************************************************
 * @brief HID LED state callback (from host)
 *
 * @param[in] leds  LED state bitmask
 ****************************************************************************************
 */
void usbd_hid_leds(uint8_t leds)
{
    if (leds & 0x02) /* CAPS_LOCK */
        GPIO_DAT_CLR(LED2);
    else
        GPIO_DAT_SET(LED2);
}

/**
 ****************************************************************************************
 * @brief USB HID test with physical buttons
 ****************************************************************************************
 */
void usbdTest(void)
{
    static uint32_t btn_lvl = BTNS;
    uint8_t ret = 0;
    uint32_t value = GPIO_PIN_GET() & BTNS;
    uint32_t chng = btn_lvl ^ value;
    btn_lvl = value;

    if (chng)
    {
        uint8_t code = 0;

        usbd_wakeup();

        if ((chng & BTN1) && ((value & BTN1) == 0))
            code = HID_KEY_A;
        if ((chng & BTN2) && ((value & BTN2) == 0))
            code = HID_KEY_B;
        if ((chng & BTN3) && ((value & BTN3) == 0))
            code = HID_KEY_C;

        debug("keys(val:%X,chng:%X,code:%d)\r\n", btn_lvl, chng, code);
        ret = hid_keybd_send_report(code);
        if (ret != 0)
            debug("keys Fail(sta:%d)\r\n", ret);

        if (code)
        {
            GPIO_DAT_CLR(LED1);
            hid_mouse_send_report(100);
        }
        else
        {
            GPIO_DAT_SET(LED1);
            hid_mouse_send_report(-100);
        }
    }
}

#else /* !USE_KEYS - auto circle test */

/// Circle movement X offsets
static const int8_t x_offset[] = {
    -1,-2,-4,-5,-6,-7,-8,-9,-8,-8,-9,-8,-7,-6,-5,-4,-2,-1,
     1, 2, 4, 5, 6, 7, 8, 9, 8,8,9,8,7,6,5,4,2,1
};

/// Circle movement Y offsets
static const int8_t y_offset[] = {
     8, 9, 8, 7, 6, 5, 4, 2, 1,-1,-2,-4,-5,-6,-7,-8,-9,-8,
    -8,-9,-8,-7,-6,-5,-4,-2,-1,1,2,4,5,6,7,8,9,8
};

/**
 ****************************************************************************************
 * @brief USB HID test with auto-circle data
 ****************************************************************************************
 */
void usbdTest(void)
{
    static uint8_t keynum = 0;
    uint8_t ret;

    if (!usbd_is_configured())
        return;

#if (ENB_KEYBD)
    /* Keyboard test */
    {
        uint8_t keybd_report[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

        if ((keynum % 2) == 0)
            keybd_report[2] = HID_KEY_A + keynum / 2;

        ret = usbd_hid_send_report(KEYBD_IN_EP, 8, keybd_report);
        USB_LOG_RAW("hid_keybd(key:%d,ret:%d)\r\n", keynum, ret);
    }
#endif

#if (ENB_MOUSE)
    /* Mouse test */
    {
        uint8_t mouse_report[4] = { 0x00, 0x00, 0x00, 0x00 };

        mouse_report[1] = (x_offset[keynum] * 2);
        mouse_report[2] = (y_offset[keynum] * 2);
        ret = usbd_hid_send_report(MOUSE_IN_EP, MOUSE_IN_EP_SIZE, mouse_report);
        USB_LOG_RAW("hid_mouse(x:%d,ret:%d)\r\n", mouse_report[1], ret);
    }
#endif

    if (ret == USBD_OK)
    {
        keynum = (keynum + 1) % sizeof(x_offset);
        bootDelayUs(990);
    }
}

#endif /* USE_KEYS */

/**
 ****************************************************************************************
 * @brief Initialize USB HID boot device
 ****************************************************************************************
 */
void usbdInit(void)
{
    suspend = false;
    rcc_usb_en();
    usbd_init();
    usbd_register(hid_descriptor, hid_configuration);
    NVIC_EnableIRQ(USB_IRQn);

    for (uint8_t idx = 0; idx < USB_HID_INTF_CNT; idx++)
    {
        usbd_hid_init(idx, &hid_interface[idx]);
    }

    keys_init();
}

#endif /* DEMO_HID_BOOT */
