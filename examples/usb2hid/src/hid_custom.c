/**
 ****************************************************************************************
 *
 * @file hid_custom.c
 *
 * @brief USB HID Custom device implementation (keyboard + raw bidirectional HID)
 *
 * @details
 * DEMO_HID_CUSTOM example:
 * - Device init: similar to boot example, but with bidirectional endpoints
 * - Enumeration: host recognizes composite device with keyboard and custom raw data
 * - Bidirectional communication: send data to host AND receive LED state / custom data
 * - Data processing: handle host-sent data and perform corresponding actions
 *
 * USB HID device based on USB Human Interface Device class specification:
 * - Descriptor definition: device, config, interface, endpoint, and report descriptors
 * - Interrupt transfer: low-latency data transfer via interrupt endpoints
 * - Report mechanism: predefined report formats for device-host data exchange
 * - Protocol handling: USB core handles standard requests, HID class handles HID-specific
 ****************************************************************************************
 */

#include "usbd.h"
#include "usbd_hid.h"
#include "drvs.h"
#include "dbg.h"

#if (DEMO_HID_CUSTOM)

/*
 * DEFINES - USB Device Configuration
 ****************************************************************************************
 */

#define USBD_BCD                  USB_2_0     ///< USB specification version
#define USBD_VID                  0xFFFF      ///< Vendor ID
#define USBD_PID                  0xFFFF      ///< Product ID
#define USBD_MAX_POWER            100         ///< Max power (mA)
#define USBD_LANGID_STRING        0x0409      ///< English (US)

/*
 * DEFINES - Interface & Descriptor Sizes
 ****************************************************************************************
 */

/// Total HID interface count
#define USB_HID_INTF_CNT          2

/// Last interface number
#define USB_HID_INTF_END          (USB_HID_INTF_CNT - 1)

/// Configuration descriptor total size (in & out endpoints per interface)
#define USB_HID_CONFIG_SIZE       (9 + (18 + 7 + 7) * USB_HID_INTF_CNT)

/*
 * DEFINES - Keyboard Endpoint Configuration
 ****************************************************************************************
 */

#define KBD_INTF_NUM              0           ///< Keyboard interface number
#define KBD_IN_EP                 0x81        ///< Keyboard IN endpoint address
#define KBD_IN_EP_SIZE            8           ///< Keyboard IN max packet size
#define KBD_IN_EP_INTERVAL        10          ///< Keyboard IN polling interval (ms)
#define KBD_OUT_EP                0x01        ///< Keyboard OUT endpoint address
#define KBD_OUT_EP_SIZE           1           ///< Keyboard OUT max packet size
/// Endpoint polling interval: 1, 2, 4, 8, 16, ... 2^n
#define KBD_OUT_EP_INTERVAL       8
#define KBD_REPORT_DESC_SIZE      sizeof(hid_kbd_report_desc)

/*
 * DEFINES - Custom Raw Endpoint Configuration
 ****************************************************************************************
 */

#define RAW_INTF_NUM              1           ///< Raw data interface number
#define RAW_IN_EP                 0x82        ///< Raw data IN endpoint address
#define RAW_IN_EP_SIZE            64          ///< Raw data IN max packet size
#define RAW_IN_EP_INTERVAL        10          ///< Raw data IN polling interval (ms)
#define RAW_OUT_EP                0x02        ///< Raw data OUT endpoint address
#define RAW_OUT_EP_SIZE           64          ///< Raw data OUT max packet size
/// Endpoint polling interval: 1, 2, 4, 8, 16, ... 2^n
#define RAW_OUT_EP_INTERVAL       8
#define RAW_REPORT_DESC_SIZE      sizeof(hid_raw_report_desc)

/*
 * Forward Declarations
 ****************************************************************************************
 */

void usbd_hid_kbd_out_handler(uint8_t ep);
void usbd_hid_raw_out_handler(uint8_t ep);

/*
 * LOCAL DATA - Report Descriptors
 ****************************************************************************************
 */

/// HID keyboard report descriptor
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

/// HID custom raw report descriptor (vendor-defined, bidirectional)
static const uint8_t hid_raw_report_desc[] = {
    0x06, 0x00, 0xFF, // Usage Page (Vendor Defined 0xFF00)
    0x09, 0x01,       // Usage (0x01)
    0xA1, 0x01,       // Collection (Application)
    0x09, 0x01,       //   Usage (0x01)
    0x15, 0x00,       //   Logical Minimum (0)
    0x26, 0xFF, 0x00, //   Logical Maximum (255)
    0x95, 0x40,       //   Report Count (64)
    0x75, 0x08,       //   Report Size (8)
    0x81, 0x02,       //   Input (Data,Var,Abs)
    0x09, 0x01,       //   Usage (0x01)
    0x15, 0x00,       //   Logical Minimum (0)
    0x26, 0xFF, 0x00, //   Logical Maximum (255)
    0x95, 0x40,       //   Report Count (64)
    0x75, 0x08,       //   Report Size (8)
    0x91, 0x02,       //   Output (Data,Var,Abs)
    0xC0,             // End Collection
};

/*
 * LOCAL DATA - USB Device Descriptor
 ****************************************************************************************
 */

/// Full HID device descriptor (device + config + interfaces + strings)
static const uint8_t hid_descriptor[] = {
    /* Device descriptor (18 bytes) */
    USB_DEVICE_DESCRIPTOR_INIT(USBD_BCD, 0x00, 0x00, 0x00, USBD_VID, USBD_PID, 0x0002, 0x01),

    /* Configuration descriptor */
    USB_CONFIG_DESCRIPTOR_INIT(USB_HID_CONFIG_SIZE, USB_HID_INTF_CNT,
            0x01, USB_CONFIG_BUS_POWERED | USB_CONFIG_REMOTE_WAKEUP, USBD_MAX_POWER),

    /* Keyboard interface (18 + 7*2 bytes) */
    HID_INTERFACE_INIT(KBD_INTF_NUM, 2, HID_SUBCLASS_BOOTIF, HID_PROTOCOL_KEYBOARD, 0, KBD_REPORT_DESC_SIZE),
    HID_ENDPOINT_DESC(KBD_IN_EP, KBD_IN_EP_SIZE, KBD_IN_EP_INTERVAL),
    HID_ENDPOINT_DESC(KBD_OUT_EP, KBD_OUT_EP_SIZE, KBD_OUT_EP_INTERVAL),

    /* Custom raw interface (18 + 7*2 bytes) */
    HID_INTERFACE_INIT(RAW_INTF_NUM, 2, HID_SUBCLASS_BOOTIF, HID_PROTOCOL_NONE, 0, RAW_REPORT_DESC_SIZE),
    HID_ENDPOINT_DESC(RAW_IN_EP, RAW_IN_EP_SIZE, RAW_IN_EP_INTERVAL),
    HID_ENDPOINT_DESC(RAW_OUT_EP, RAW_OUT_EP_SIZE, RAW_OUT_EP_INTERVAL),

    /* String descriptors */
    // String 0 - Language ID (4 bytes)
    USB_LANGID_INIT(USBD_LANGID_STRING),

    // String 1 - iManufacturer
    0x02,
    USB_DESC_TYPE_STRING,

    // String 2 - iProduct: "USB RawHID"
    0x16,
    USB_DESC_TYPE_STRING,
    WCHAR('U'), WCHAR('S'), WCHAR('B'), WCHAR(' '),
    WCHAR('R'), WCHAR('a'), WCHAR('w'), WCHAR('H'),
    WCHAR('I'), WCHAR('D'),

    /* Device qualifier (USB 2.0 only) */
#if (USBD_BCD == USB_2_0)
    USB_QUALIFIER_INIT(0x01),
#endif

    /* End of descriptor */
    0x00
};

/*
 * LOCAL DATA - Configuration Tables
 ****************************************************************************************
 */

/// HID interface table
static const hid_intf_t hid_interface[] = {
    HID_INTF_T(KBD_INTF_NUM, KBD_IN_EP, hid_kbd_report_desc),
    HID_INTF_T(RAW_INTF_NUM, RAW_IN_EP, hid_raw_report_desc),
};

/// Endpoint table
static const usbd_ep_t endpoint_tab[] = {
    USBD_EP_T(KBD_IN_EP,  USB_EP_TYPE_INTERRUPT, KBD_IN_EP_SIZE,  &usbd_hid_ep_in_handler),
    USBD_EP_T(KBD_OUT_EP, USB_EP_TYPE_INTERRUPT, KBD_OUT_EP_SIZE, &usbd_hid_kbd_out_handler),

    USBD_EP_T(RAW_IN_EP,  USB_EP_TYPE_INTERRUPT, RAW_IN_EP_SIZE,  &usbd_hid_ep_in_handler),
    USBD_EP_T(RAW_OUT_EP, USB_EP_TYPE_INTERRUPT, RAW_OUT_EP_SIZE, &usbd_hid_raw_out_handler),
};

/// Class handler table
static const usbd_class_t class_tab[] = {
    USBD_CLASS_T(0, USB_HID_INTF_END, &usbd_hid_class_handler),
};

/// USBD Configuration
static const usbd_config_t hid_configuration[] = {
    USBD_CONFIG_T(1, USB_HID_INTF_CNT, class_tab, endpoint_tab)
};

/*
 * CALLBACK FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Keyboard OUT endpoint handler (receives LED state from host)
 *
 * @param[in] ep  Endpoint address
 ****************************************************************************************
 */
void usbd_hid_kbd_out_handler(uint8_t ep)
{
    uint8_t led_state;

    /* Read LED data sent by host */
    usbd_ep_read(ep, KBD_OUT_EP_SIZE, &led_state);
    USB_LOG_RAW("led_state:%02X\r\n", led_state);

    /* Process LED state */
    usbd_hid_leds(led_state);
}

/**
 ****************************************************************************************
 * @brief Raw data OUT endpoint handler (receives custom data from host)
 *
 * @param[in] ep  Endpoint address (unused)
 ****************************************************************************************
 */
void usbd_hid_raw_out_handler(uint8_t ep)
{
    (void)ep;
    uint8_t custom_data[RAW_OUT_EP_SIZE];

    /* Read custom data from host */
    usbd_ep_read(RAW_OUT_EP, RAW_OUT_EP_SIZE, custom_data);

    /* Process custom data here */
}

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
            usbd_hid_reset();
            break;

        case USBD_EVENT_SUSPEND:
        case USBD_EVENT_RESUME:
            break;

        default:
            break;
    }
}

/*
 * PUBLIC FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialize USB HID custom device
 ****************************************************************************************
 */
void usbdInit(void)
{
    rcc_usb_en();
    usbd_init();
    usbd_register(hid_descriptor, hid_configuration);
    NVIC_EnableIRQ(USB_IRQn);

    for (uint8_t idx = 0; idx < USB_HID_INTF_CNT; idx++)
    {
        usbd_hid_init(idx, &hid_interface[idx]);
    }
}

/**
 ****************************************************************************************
 * @brief USB HID custom test function (call in main loop)
 *
 * @details
 * Cycles through keyboard keys and sends raw data patterns.
 * Each iteration: key-down, key-up, then 64-byte raw pattern.
 ****************************************************************************************
 */
void usbdTest(void)
{
    static uint8_t keynum = 0;

    if (!usbd_is_configured())
        return;

    /* Keyboard test: send key down */
    {
        uint8_t sendbuffer[KBD_IN_EP_SIZE] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

        sendbuffer[2] = HID_KEY_A + keynum;
        usbd_hid_send_report(KBD_IN_EP, KBD_IN_EP_SIZE, sendbuffer);
        bootDelayMs(10);

        /* Send key up */
        sendbuffer[2] = 0;
        usbd_hid_send_report(KBD_IN_EP, KBD_IN_EP_SIZE, sendbuffer);
    }

    /* Raw data test: send 64-byte pattern */
    {
        uint8_t sendbuffer[RAW_IN_EP_SIZE];
        bootDelayMs(100);

        memset(sendbuffer, keynum, RAW_IN_EP_SIZE);
        usbd_hid_send_report(RAW_IN_EP, RAW_IN_EP_SIZE, sendbuffer);
    }

    if (++keynum > 94)
        keynum = 0;
}

#endif /* DEMO_HID_CUSTOM */
