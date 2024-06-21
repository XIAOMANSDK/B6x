/**
 ****************************************************************************************
 *
 * @file usb_def.h
 *
 * @brief Header file of USB Definitions(types and macros and structs)
 *
 ****************************************************************************************
 */

#ifndef _USB_DEF_H
#define _USB_DEF_H

#include <stdint.h>
#include <stdbool.h>


/*******************************************************************************
 *                 CMSIS Util definitions
 ******************************************************************************/

#if defined(__CC_ARM)
#ifndef __USED
    #define __USED          __attribute__((used))
#endif
#ifndef __WEAK
    #define __WEAK          __attribute__((weak))
#endif
#ifndef __PACKED
    #define __PACKED        __attribute__((packed))
#endif
#ifndef __PACKED_STRUCT
    #define __PACKED_STRUCT __packed struct
#endif
#ifndef __PACKED_UNION
    #define __PACKED_UNION  __packed union
#endif
#ifndef __ALIGNED
    #define __ALIGNED(x)    __attribute__((aligned(x)))
#endif
#elif defined(__GNUC__)
#ifndef __USED
    #define __USED          __attribute__((used))
#endif
#ifndef __WEAK
    #define __WEAK          __attribute__((weak))
#endif
#ifndef __PACKED
    #define __PACKED        __attribute__((packed, aligned(1)))
#endif
#ifndef __PACKED_STRUCT
    #define __PACKED_STRUCT struct __attribute__((packed, aligned(1)))
#endif
#ifndef __PACKED_UNION
    #define __PACKED_UNION  union __attribute__((packed, aligned(1)))
#endif
#ifndef __ALIGNED
    #define __ALIGNED(x)    __attribute__((aligned(x)))
#endif
#elif defined(__ICCARM__)
#ifndef __USED
#if __ICCARM_V8
    #define __USED          __attribute__((used))
#else
    #define __USED          _Pragma("__root")
#endif
#endif

#ifndef __WEAK
#if __ICCARM_V8
    #define __WEAK          __attribute__((weak))
#else
    #define __WEAK          _Pragma("__weak")
#endif
#endif

#ifndef __PACKED
#if __ICCARM_V8
    #define __PACKED        __attribute__((packed, aligned(1)))
#else
    #define __PACKED        __packed /* Needs IAR language extensions */
#endif
#endif

#ifndef __PACKED_STRUCT
#if __ICCARM_V8
    #define __PACKED_STRUCT struct __attribute__((packed, aligned(1)))
#else
    #define __PACKED_STRUCT __packed struct /* Needs IAR language extensions */
#endif
#endif

#ifndef __PACKED_UNION
#if __ICCARM_V8
    #define __PACKED_UNION  union __attribute__((packed, aligned(1)))
#else
    #define __PACKED_UNION  __packed union /* Needs IAR language extensions */
#endif
#endif

#ifndef __ALIGNED
#if __ICCARM_V8
    #define __ALIGNED(x)    __attribute__((aligned(x)))
#elif (__VER__ >= 7080000)
    #define __ALIGNED(x)    __attribute__((aligned(x))) /* Needs IAR language extensions */
#else
    #warning No compiler specific solution for __ALIGNED.__ALIGNED is ignored.
    #define __ALIGNED(x)
#endif
#endif

#endif

#ifndef __ALIGN_BEGIN
    #define __ALIGN_BEGIN
#endif
#ifndef __ALIGN_END
    #define __ALIGN_END     __attribute__((aligned(4)))
#endif

#define __USBIRQ           __attribute__((section("usb_irq")))
#define __USBDESC          __attribute__((section("usb_desc"))) __USED __ALIGNED(1)

// The unused argument @p x.
#ifndef ARG_UNUSED
#define ARG_UNUSED(x)       (void)(x)
#endif
// The low byte of 16bits @p x.
#ifndef LO_BYTE
#define LO_BYTE(x)          ((uint8_t)(x & 0x00FF))
#endif
// The high byte of 16bits @p x.
#ifndef HI_BYTE
#define HI_BYTE(x)          ((uint8_t)((x & 0xFF00) >> 8))
#endif
// The larger value between @p a and @p b.
#ifndef MAX
#define MAX(a, b)           (((a) > (b)) ? (a) : (b))
#endif
// The smaller value between @p a and @p b.
#ifndef MIN
#define MIN(a, b)           (((a) < (b)) ? (a) : (b))
#endif
// The Binary-Coded Decimal @p x.
#ifndef BCD
#define BCD(x)              ((((x) / 10) << 4) | ((x) % 10))
#endif
// The bit of left-shift @p n.
#ifndef BIT
#define BIT(n)              (1UL << (n))
#endif
// The elements count in @p array.
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array)   ((sizeof(array) / sizeof((array)[0])))
#endif
// Bytes swap @p u16.
#ifndef BSWAP16
#define BSWAP16(u16)        (__builtin_bswap16(u16))
#endif
// Bytes swap @p u32.
#ifndef BSWAP32
#define BSWAP32(u32)        (__builtin_bswap32(u32))
#endif

// Get or Set BigEndian value
#define GET_BE16(field) \
    (((uint16_t)(field)[0] << 8) | ((uint16_t)(field)[1]))

#define GET_BE32(field) \
    (((uint32_t)(field)[0] << 24) | ((uint32_t)(field)[1] << 16) | ((uint32_t)(field)[2] << 8) | ((uint32_t)(field)[3] << 0))

#define SET_BE16(field, value)                \
    do {                                      \
        (field)[0] = (uint8_t)((value) >> 8); \
        (field)[1] = (uint8_t)((value) >> 0); \
    } while (0)

#define SET_BE24(field, value)                 \
    do {                                       \
        (field)[0] = (uint8_t)((value) >> 16); \
        (field)[1] = (uint8_t)((value) >> 8);  \
        (field)[2] = (uint8_t)((value) >> 0);  \
    } while (0)

#define SET_BE32(field, value)                 \
    do {                                       \
        (field)[0] = (uint8_t)((value) >> 24); \
        (field)[1] = (uint8_t)((value) >> 16); \
        (field)[2] = (uint8_t)((value) >> 8);  \
        (field)[3] = (uint8_t)((value) >> 0);  \
    } while (0)

// Convert value to byte elements.
#define WCHAR(c)            (c & 0xFF), ((c >> 8) & 0xFF)
#define WBVAL(x)            (x & 0xFF), ((x >> 8) & 0xFF)
#define DBVAL(x)            (x & 0xFF), ((x >> 8) & 0xFF), ((x >> 16) & 0xFF), ((x >> 24) & 0xFF)

// Type and index of the descriptor
#define GET_DESC_TYPE(x)    (((x) >> 8) & 0xFF)
#define GET_DESC_INDEX(x)   ((x) & 0xFF)

// The Number of arguments, max 63.
#define PP_NARG(...) \
    PP_NARG_(__VA_ARGS__, PP_RSEQ_N())
#define PP_NARG_(...) \
    PP_ARG_N(__VA_ARGS__)
#define PP_ARG_N(                                     \
    _1, _2, _3, _4, _5, _6, _7, _8, _9, _10,          \
    _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, \
    _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, \
    _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
    _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, N, ...) N
#define PP_RSEQ_N()                         \
    63, 62, 61, 60,                         \
    59, 58, 57, 56, 55, 54, 53, 52, 51, 50, \
    49, 48, 47, 46, 45, 44, 43, 42, 41, 40, \
    39, 38, 37, 36, 35, 34, 33, 32, 31, 30, \
    29, 28, 27, 26, 25, 24, 23, 22, 21, 20, \
    19, 18, 17, 16, 15, 14, 13, 12, 11, 10, \
    9, 8, 7, 6, 5, 4, 3, 2, 1, 0


/*******************************************************************************
 *                 USB Core definitions
 ******************************************************************************/

/**< maximum packet size (MPS) for EP 0~NUM */
#ifndef USB_EP0_MPS
    #define USB_EP0_MPS                  64
#endif

#ifndef USB_EP_MPS
    #define USB_EP_MPS                   64
#endif

/* Version define */
#define USB_1_1                          0x0110
#define USB_2_0                          0x0200
#define USB_2_1                          0x0210

/* Device speeds */
#define USB_SPEED_UNKNOWN                0 /* Transfer rate not yet set */
#define USB_SPEED_LOW                    1 /* USB 1.1 */
#define USB_SPEED_FULL                   2 /* USB 1.1 */
#define USB_SPEED_HIGH                   3 /* USB 2.0 */
#define USB_SPEED_VARIABLE               4 /* Wireless USB 2.5 */

// USB PID Types
#define USB_PID_OUT                      (0x01) /* Tokens */
#define USB_PID_IN                       (0x09)
#define USB_PID_SOF                      (0x05)
#define USB_PID_SETUP                    (0x0d)

#define USB_PID_DATA0                    (0x03) /* Data */
#define USB_PID_DATA1                    (0x0b)
#define USB_PID_DATA2                    (0x07)
#define USB_PID_MDATA                    (0x0f)

#define USB_PID_ACK                      (0x02) /* Handshake */
#define USB_PID_NAK                      (0x0a)
#define USB_PID_STALL                    (0x0e)
#define USB_PID_NYET                     (0x06)

#define USB_PID_PRE                      (0x0c) /* Special */
#define USB_PID_ERR                      (0x0c)
#define USB_PID_SPLIT                    (0x08)
#define USB_PID_PING                     (0x04)
#define USB_PID_RESERVED                 (0x00)

#define USB_REQUEST_DIR_LSB              7U /* Bits 7: Request dir */
#define USB_REQUEST_DIR_OUT              (0U << USB_REQUEST_DIR_LSB) /* Bit 7=0: Host-to-device */
#define USB_REQUEST_DIR_IN               (1U << USB_REQUEST_DIR_LSB) /* Bit 7=1: Device-to-host */
#define USB_REQUEST_DIR_MASK             (1U << USB_REQUEST_DIR_LSB) /* Bit 7=1: Direction bit */

#define USB_REQUEST_TYPE_LSB             5U /* Bits 5:6: Request type */
#define USB_REQUEST_STANDARD             (0U << USB_REQUEST_TYPE_LSB)
#define USB_REQUEST_CLASS                (1U << USB_REQUEST_TYPE_LSB)
#define USB_REQUEST_VENDOR               (2U << USB_REQUEST_TYPE_LSB)
#define USB_REQUEST_RESERVED             (3U << USB_REQUEST_TYPE_LSB)
#define USB_REQUEST_TYPE_MASK            (3U << USB_REQUEST_TYPE_LSB)

#define USB_REQUEST_RECIP_LSB            0U /* Bits 0:4: Recipient */
#define USB_REQUEST_RECIP_DEVICE         (0U << USB_REQUEST_RECIP_LSB)
#define USB_REQUEST_RECIP_INTERFACE      (1U << USB_REQUEST_RECIP_LSB)
#define USB_REQUEST_RECIP_ENDPOINT       (2U << USB_REQUEST_RECIP_LSB)
#define USB_REQUEST_RECIP_OTHER          (3U << USB_REQUEST_RECIP_LSB)
#define USB_REQUEST_RECIP_MASK           (3U << USB_REQUEST_RECIP_LSB)

/* USB Standard Request Codes */
#define USB_REQUEST_GET_STATUS           0x00
#define USB_REQUEST_CLEAR_FEATURE        0x01
#define USB_REQUEST_SET_FEATURE          0x03
#define USB_REQUEST_SET_ADDRESS          0x05
#define USB_REQUEST_GET_DESCRIPTOR       0x06
#define USB_REQUEST_SET_DESCRIPTOR       0x07
#define USB_REQUEST_GET_CONFIGURATION    0x08
#define USB_REQUEST_SET_CONFIGURATION    0x09
#define USB_REQUEST_GET_INTERFACE        0x0A
#define USB_REQUEST_SET_INTERFACE        0x0B
#define USB_REQUEST_SYNCH_FRAME          0x0C
#define USB_REQUEST_SET_ENCRYPTION       0x0D
#define USB_REQUEST_GET_ENCRYPTION       0x0E
#define USB_REQUEST_RPIPE_ABORT          0x0E
#define USB_REQUEST_SET_HANDSHAKE        0x0F
#define USB_REQUEST_RPIPE_RESET          0x0F
#define USB_REQUEST_GET_HANDSHAKE        0x10
#define USB_REQUEST_SET_CONNECTION       0x11
#define USB_REQUEST_SET_SECURITY_DATA    0x12
#define USB_REQUEST_GET_SECURITY_DATA    0x13
#define USB_REQUEST_SET_WUSB_DATA        0x14
#define USB_REQUEST_LOOPBACK_DATA_WRITE  0x15
#define USB_REQUEST_LOOPBACK_DATA_READ   0x16
#define USB_REQUEST_SET_INTERFACE_DS     0x17

/* USB Standard Feature selectors */
#define USB_FEATURE_ENDPOINT_HALT        0
#define USB_FEATURE_SELF_POWERED         0
#define USB_FEATURE_REMOTE_WAKEUP        1
#define USB_FEATURE_TEST_MODE            2
#define USB_FEATURE_BATTERY              2
#define USB_FEATURE_BHNPENABLE           3
#define USB_FEATURE_WUSBDEVICE           3
#define USB_FEATURE_AHNPSUPPORT          4
#define USB_FEATURE_AALTHNPSUPPORT       5
#define USB_FEATURE_DEBUGMODE            6

/* USB GET_STATUS Bit Values */
#define USB_GETSTATUS_ENDPOINT_HALT      0x01
#define USB_GETSTATUS_SELF_POWERED       0x01
#define USB_GETSTATUS_REMOTE_WAKEUP      0x02

/* USB Descriptor Types */
#define USB_DESC_TYPE_DEVICE                 0x01U
#define USB_DESC_TYPE_CONFIGURATION          0x02U
#define USB_DESC_TYPE_STRING                 0x03U
#define USB_DESC_TYPE_INTERFACE              0x04U
#define USB_DESC_TYPE_ENDPOINT               0x05U
#define USB_DESC_TYPE_DEVICE_QUALIFIER       0x06U
#define USB_DESC_TYPE_OTHER_SPEED            0x07U
#define USB_DESC_TYPE_INTERFACE_POWER        0x08U
#define USB_DESC_TYPE_OTG                    0x09U
#define USB_DESC_TYPE_DEBUG                  0x0AU
#define USB_DESC_TYPE_INTERFACE_ASSOCIATION  0x0BU
#define USB_DESC_TYPE_BINARY_OBJECT_STORE    0x0FU
#define USB_DESC_TYPE_DEVICE_CAPABILITY      0x10U
#define USB_DESC_TYPE_WIRELESS_ENDPOINTCOMP  0x11U

/* Class Specific Descriptor */
#define USB_CLASS_DESC_TYPE_DEVICE           0x21U
#define USB_CLASS_DESC_TYPE_CONFIGURATION    0x22U
#define USB_CLASS_DESC_TYPE_STRING           0x23U
#define USB_CLASS_DESC_TYPE_INTERFACE        0x24U
#define USB_CLASS_DESC_TYPE_ENDPOINT         0x25U

#define USB_DESC_TYPE_SUPERSPEED_ENDPOINT_COMPANION      0x30U
#define USB_DESC_TYPE_SUPERSPEED_ISO_ENDPOINT_COMPANION  0x31U

/* USB Device Classes */
#define USB_DEVICE_CLASS_RESERVED        0x00
#define USB_DEVICE_CLASS_AUDIO           0x01
#define USB_DEVICE_CLASS_CDC             0x02
#define USB_DEVICE_CLASS_HID             0x03
#define USB_DEVICE_CLASS_MONITOR         0x04
#define USB_DEVICE_CLASS_PHYSICAL        0x05
#define USB_DEVICE_CLASS_IMAGE           0x06
#define USB_DEVICE_CLASS_PRINTER         0x07
#define USB_DEVICE_CLASS_MASS_STORAGE    0x08
#define USB_DEVICE_CLASS_HUB             0x09
#define USB_DEVICE_CLASS_CDC_DATA        0x0a
#define USB_DEVICE_CLASS_SMART_CARD      0x0b
#define USB_DEVICE_CLASS_SECURITY        0x0d
#define USB_DEVICE_CLASS_VIDEO           0x0e
#define USB_DEVICE_CLASS_HEALTHCARE      0x0f
#define USB_DEVICE_CLASS_DIAG_DEVICE     0xdc
#define USB_DEVICE_CLASS_WIRELESS        0xe0
#define USB_DEVICE_CLASS_MISC            0xef
#define USB_DEVICE_CLASS_APP_SPECIFIC    0xfe
#define USB_DEVICE_CLASS_VEND_SPECIFIC   0xff

/* usb string index define */
#define USB_STRING_LANGID_INDEX          0x00
#define USB_STRING_MFC_INDEX             0x01
#define USB_STRING_PRODUCT_INDEX         0x02
#define USB_STRING_SERIAL_INDEX          0x03
#define USB_STRING_CONFIG_INDEX          0x04
#define USB_STRING_INTERFACE_INDEX       0x05
#define USB_STRING_OS_INDEX              0x06
#define USB_STRING_MAX                   USB_STRING_OS_INDEX
/*
 * Devices supporting Microsoft OS Descriptors store special string
 * descriptor at fixed index (0xEE). It is read when a new device is
 * attached to a computer for the first time.
 */
#define USB_OSDESC_STRING_DESC_INDEX     0xEE

/* bmAttributes in Configuration Descriptor */
#define USB_CONFIG_REMOTE_WAKEUP         0x20
#define USB_CONFIG_POWERED_MASK          0x40
#define USB_CONFIG_BUS_POWERED           0x80
#define USB_CONFIG_SELF_POWERED          0xC0

/* bMaxPower in Configuration Descriptor */
#define USB_CONFIG_POWER_MA(mA)          ((mA) / 2)

/* bEndpointAddress in Endpoint Descriptor */
#define USB_EP_DIR_MASK                  0x80
#define USB_EP_DIR_IN                    0x80
#define USB_EP_DIR_OUT                   0x00
#define USB_EP_OUT(idx)                  ((idx) | USB_EP_DIR_OUT)
#define USB_EP_IN(idx)                   ((idx) | USB_EP_DIR_IN)

/**
 * USB endpoint direction and number.
 */
/** Get endpoint index (number) from endpoint address */
#define USB_EP_GET_IDX(ep)               ((ep) & ~USB_EP_DIR_MASK)
/** Get direction from endpoint address */
#define USB_EP_GET_DIR(ep)               ((ep) & USB_EP_DIR_MASK)
/** Get endpoint address from endpoint index and direction */
#define USB_EP_GET_ADDR(idx, dir)        ((idx) | ((dir) & USB_EP_DIR_MASK))
/** True if the endpoint is an IN endpoint */
#define USB_EP_DIR_IS_IN(ep)             (USB_EP_GET_DIR(ep) == USB_EP_DIR_IN)
/** True if the endpoint is an OUT endpoint */
#define USB_EP_DIR_IS_OUT(ep)            (USB_EP_GET_DIR(ep) == USB_EP_DIR_OUT)

/* bmAttributes in Endpoint Descriptor */
#define USB_EP_TYPE_LSB                  0
#define USB_EP_TYPE_CONTROL              (0 << USB_EP_TYPE_LSB)
#define USB_EP_TYPE_ISOCHRONOUS          (1 << USB_EP_TYPE_LSB)
#define USB_EP_TYPE_BULK                 (2 << USB_EP_TYPE_LSB)
#define USB_EP_TYPE_INTERRUPT            (3 << USB_EP_TYPE_LSB)
#define USB_EP_TYPE_MASK                 (3 << USB_EP_TYPE_LSB)

#define USB_EP_SYNC_LSB                  2
#define USB_EP_SYNC_NO_SYNCHRONIZATION   (0 << USB_EP_SYNC_LSB)
#define USB_EP_SYNC_ASYNCHRONOUS         (1 << USB_EP_SYNC_LSB)
#define USB_EP_SYNC_ADAPTIVE             (2 << USB_EP_SYNC_LSB)
#define USB_EP_SYNC_SYNCHRONOUS          (3 << USB_EP_SYNC_LSB)
#define USB_EP_SYNC_MASK                 (3 << USB_EP_SYNC_LSB)

#define USB_EP_USAGE_LSB                 4
#define USB_EP_USAGE_DATA                (0 << USB_EP_USAGE_LSB)
#define USB_EP_USAGE_FEEDBACK            (1 << USB_EP_USAGE_LSB)
#define USB_EP_USAGE_IMPLICIT_FEEDBACK   (2 << USB_EP_USAGE_LSB)
#define USB_EP_USAGE_MASK                (3 << USB_EP_USAGE_LSB)

#define USB_EP_MAX_ADJUSTABLE            (1 << 7)

/* wMaxPacketSize in Endpoint Descriptor */
#define USB_MAXPACKETSIZE_LSB                      0
#define USB_MAXPACKETSIZE_MASK                     (0x7ff << USB_MAXPACKETSIZE_LSB)
#define USB_MAXPACKETSIZE_EXTRA_TRANSCATION_LSB    11
#define USB_MAXPACKETSIZE_EXTRA_TRANSCATION_NONE   (0 << USB_MAXPACKETSIZE_ADDITIONAL_TRANSCATION_LSB)
#define USB_MAXPACKETSIZE_EXTRA_TRANSCATION_ONE    (1 << USB_MAXPACKETSIZE_ADDITIONAL_TRANSCATION_LSB)
#define USB_MAXPACKETSIZE_EXTRA_TRANSCATION_TWO    (2 << USB_MAXPACKETSIZE_ADDITIONAL_TRANSCATION_LSB)
#define USB_MAXPACKETSIZE_EXTRA_TRANSCATION_MASK   (3 << USB_MAXPACKETSIZE_ADDITIONAL_TRANSCATION_LSB)

/* bDevCapabilityType in Device Capability Descriptor */
#define USB_DEVICE_CAP_WIRELESS_USB                1
#define USB_DEVICE_CAP_USB_2_0_EXTENSION           2
#define USB_DEVICE_CAP_SUPERSPEED_USB              3
#define USB_DEVICE_CAP_CONTAINER_ID                4
#define USB_DEVICE_CAP_PLATFORM                    5
#define USB_DEVICE_CAP_POWER_DELIVERY_CAPABILITY   6
#define USB_DEVICE_CAP_BATTERY_INFO_CAPABILITY     7
#define USB_DEVICE_CAP_PD_CONSUMER_PORT_CAPABILITY 8
#define USB_DEVICE_CAP_PD_PROVIDER_PORT_CAPABILITY 9
#define USB_DEVICE_CAP_SUPERSPEED_PLUS             10
#define USB_DEVICE_CAP_PRECISION_TIME_MEASUREMENT  11
#define USB_DEVICE_CAP_WIRELESS_USB_EXT            12

#define USB_BOS_CAP_EXTENSION                      0x02
#define USB_BOS_CAP_PLATFORM                       0x05

/* OTG SET FEATURE Constants */
#define USB_OTG_FEATURE_B_HNP_ENABLE               3 /* Enable B device to perform HNP */
#define USB_OTG_FEATURE_A_HNP_SUPPORT              4 /* A device supports HNP */
#define USB_OTG_FEATURE_A_ALT_HNP_SUPPORT          5 /* Another port on the A device supports HNP */

/* WinUSB Microsoft OS 2.0 descriptor request codes */
#define WINUSB_REQUEST_GET_DESCRIPTOR_SET          0x07
#define WINUSB_REQUEST_SET_ALT_ENUM                0x08

/* WinUSB Microsoft OS 2.0 descriptor sizes */
#define WINUSB_DESCRIPTOR_SET_HEADER_SIZE          10
#define WINUSB_FUNCTION_SUBSET_HEADER_SIZE         8
#define WINUSB_FEATURE_COMPATIBLE_ID_SIZE          20

/* WinUSB Microsoft OS 2.0 Descriptor Types */
#define WINUSB_SET_HEADER_DESCRIPTOR_TYPE          0x00
#define WINUSB_SUBSET_HEADER_CONFIGURATION_TYPE    0x01
#define WINUSB_SUBSET_HEADER_FUNCTION_TYPE         0x02
#define WINUSB_FEATURE_COMPATIBLE_ID_TYPE          0x03
#define WINUSB_FEATURE_REG_PROPERTY_TYPE           0x04
#define WINUSB_FEATURE_MIN_RESUME_TIME_TYPE        0x05
#define WINUSB_FEATURE_MODEL_ID_TYPE               0x06
#define WINUSB_FEATURE_CCGP_DEVICE_TYPE            0x07

#define WINUSB_PROP_DATA_TYPE_REG_SZ               0x01
#define WINUSB_PROP_DATA_TYPE_REG_MULTI_SZ         0x07

/* WebUSB Descriptor Types */
#define WEBUSB_DESCRIPTOR_SET_HEADER_TYPE          0x00
#define WEBUSB_CONFIGURATION_SUBSET_HEADER_TYPE    0x01
#define WEBUSB_FUNCTION_SUBSET_HEADER_TYPE         0x02
#define WEBUSB_URL_TYPE                            0x03

/* WebUSB Request Codes */
#define WEBUSB_REQUEST_GET_URL                     0x02

/* bScheme in URL descriptor */
#define WEBUSB_URL_SCHEME_HTTP                     0x00
#define WEBUSB_URL_SCHEME_HTTPS                    0x01

/* WebUSB Descriptor sizes */
#define WEBUSB_DESCRIPTOR_SET_HEADER_SIZE          5
#define WEBUSB_CONFIGURATION_SUBSET_HEADER_SIZE    4
#define WEBUSB_FUNCTION_SUBSET_HEADER_SIZE         3


/* Setup packet definition used to read raw data from USB line */
struct usb_setup_packet {
    /** Request type. Bits 0:4 determine recipient, see
	 * \ref usb_request_recipient. Bits 5:6 determine type, see
	 * \ref usb_request_type. Bit 7 determines data transfer direction, see
	 * \ref usb_endpoint_direction.
	 */
    uint8_t  bmRequestType;

    /** Request. If the type bits of bmRequestType are equal to
	 * \ref usb_request_type::LIBUSB_REQUEST_TYPE_STANDARD
	 * "USB_REQUEST_TYPE_STANDARD" then this field refers to
	 * \ref usb_standard_request. For other cases, use of this field is
	 * application-specific. */
    uint8_t  bRequest;

    /** Value. Varies according to request */
    uint16_t wValue;

    /** Index. Varies according to request, typically used to pass an index
	 * or offset */
    uint16_t wIndex;

    /** Number of bytes to transfer */
    uint16_t wLength;
} __PACKED;

#define USB_SETUP_PACKET_SIZE     8

/** USB descriptor header */
struct usb_desc_header {
    uint8_t  bLength;             /* descriptor length */
    uint8_t  bDescriptorType;     /* descriptor type */
};

/** Standard Device Descriptor */
struct usb_device_descriptor {
    uint8_t  bLength;             /* Descriptor size in bytes = 18 */
    uint8_t  bDescriptorType;     /* DEVICE descriptor type = 1 */
    uint16_t bcdUSB;              /* USB spec in BCD, e.g. 0x0200 */
    uint8_t  bDeviceClass;        /* Class code, if 0 see interface */
    uint8_t  bDeviceSubClass;     /* Sub-Class code, 0 if class = 0 */
    uint8_t  bDeviceProtocol;     /* Protocol, if 0 see interface */
    uint8_t  bMaxPacketSize0;     /* Endpoint 0 max. size */
    uint16_t idVendor;            /* Vendor ID per USB-IF */
    uint16_t idProduct;           /* Product ID per manufacturer */
    uint16_t bcdDevice;           /* Device release # in BCD */
    uint8_t  iManufacturer;       /* Index to manufacturer string */
    uint8_t  iProduct;            /* Index to product string */
    uint8_t  iSerialNumber;       /* Index to serial number string */
    uint8_t  bNumConfigurations;  /* Number of possible configurations */
} __PACKED;

#define USB_DEVICE_DESC_SIZE      18

/** Standard Configuration Descriptor */
struct usb_configuration_descriptor {
    uint8_t  bLength;             /* Descriptor size in bytes = 9 */
    uint8_t  bDescriptorType;     /* CONFIGURATION type = 2 or 7 */
    uint16_t wTotalLength;        /* Length of concatenated descriptors */
    uint8_t  bNumInterfaces;      /* Number of interfaces, this config. */
    uint8_t  bConfigurationValue; /* Value to set this config. */
    uint8_t  iConfiguration;      /* Index to configuration string */
    uint8_t  bmAttributes;        /* Config. characteristics */
    uint8_t  bMaxPower;           /* Max.power from bus, 2mA units */
} __PACKED;

#define USB_CONFIG_DESC_SIZE      9

/** Standard Interface Descriptor */
struct usb_interface_descriptor {
    uint8_t  bLength;             /* Descriptor size in bytes = 9 */
    uint8_t  bDescriptorType;     /* INTERFACE descriptor type = 4 */
    uint8_t  bInterfaceNumber;    /* Interface no.*/
    uint8_t  bAlternateSetting;   /* Value to select this IF */
    uint8_t  bNumEndpoints;       /* Number of endpoints excluding 0 */
    uint8_t  bInterfaceClass;     /* Class code, 0xFF = vendor */
    uint8_t  bInterfaceSubClass;  /* Sub-Class code, 0 if class = 0 */
    uint8_t  bInterfaceProtocol;  /* Protocol, 0xFF = vendor */
    uint8_t  iInterface;          /* Index to interface string */
} __PACKED;

#define USB_INTERFACE_DESC_SIZE   9

/** Standard Endpoint Descriptor */
struct usb_endpoint_descriptor {
    uint8_t  bLength;             /* Descriptor size in bytes = 7 */
    uint8_t  bDescriptorType;     /* ENDPOINT descriptor type = 5 */
    uint8_t  bEndpointAddress;    /* Endpoint # 0 - 15 | IN/OUT */
    uint8_t  bmAttributes;        /* Transfer type */
    uint16_t wMaxPacketSize;      /* Bits 10:0 = max. packet size */
    uint8_t  bInterval;           /* Polling interval in (micro) frames */
} __PACKED;

#define USB_ENDPOINT_DESC_SIZE    7

/** Unicode (UTF16LE) String Descriptor */
struct usb_string_descriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t bString;
} __PACKED;

#define USB_STRING_LANGID_DESC_SIZE 4

/* USB Interface Association Descriptor */
struct usb_interface_association_descriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bFirstInterface;
    uint8_t  bInterfaceCount;
    uint8_t  bFunctionClass;
    uint8_t  bFunctionSubClass;
    uint8_t  bFunctionProtocol;
    uint8_t  iFunction;
} __PACKED;

#define USB_IAD_DESC_SIZE         8

/** USB device_qualifier descriptor */
struct usb_device_qualifier_descriptor {
    uint8_t  bLength;             /* Descriptor size in bytes = 10 */
    uint8_t  bDescriptorType;     /* DEVICE QUALIFIER type = 6 */
    uint16_t bcdUSB;              /* USB spec in BCD, e.g. 0x0200 */
    uint8_t  bDeviceClass;        /* Class code, if 0 see interface */
    uint8_t  bDeviceSubClass;     /* Sub-Class code, 0 if class = 0 */
    uint8_t  bDeviceProtocol;     /* Protocol, if 0 see interface */
    uint8_t  bMaxPacketSize;      /* Endpoint 0 max. size */
    uint8_t  bNumConfigurations;  /* Number of possible configurations */
    uint8_t  bReserved;           /* Reserved = 0 */
} __PACKED;

#define USB_DEVICE_QUALIFIER_DESC_SIZE 10

/* Microsoft OS function descriptor.
 * This can be used to request a specific driver (such as WINUSB) to be
 * loaded on Windows. Unlike other descriptors, it is requested by a special
 * request USB_REQ_GETMSFTOSDESCRIPTOR.
 * More details:
 *       https://msdn.microsoft.com/en-us/windows/hardware/gg463179
 *
 * The device will have exactly one "Extended Compat ID Feature Descriptor",
 * which may contain multiple "Function Descriptors" associated with
 * different interfaces.
 */

/* MS OS 1.0 string descriptor */
struct usb_msosv1_string_descriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bString[14];
    uint8_t  bMS_VendorCode;      /* Vendor Code, used for a control request */
    uint8_t  bPad;                /* Padding byte for VendorCode look as UTF16 */
} __PACKED;

/* MS OS 1.0 Header descriptor */
struct usb_msosv1_compat_id_header_descriptor {
    uint32_t dwLength;
    uint16_t bcdVersion;
    uint16_t wIndex;
    uint8_t  bCount;
    uint8_t  reserved[7];
} __PACKED;

/* MS OS 1.0 Function descriptor */
struct usb_msosv1_comp_id_function_descriptor {
    uint8_t  bFirstInterfaceNumber;
    uint8_t  reserved1;
    uint8_t  compatibleID[8];
    uint8_t  subCompatibleID[8];
    uint8_t  reserved2[6];
} __PACKED;

#define usb_msosv1_comp_id_create(x)                                         \
    struct usb_msosv1_comp_id {                                              \
        struct usb_msosv1_compat_id_header_descriptor compat_id_header;      \
        struct usb_msosv1_comp_id_function_descriptor compat_id_function[x]; \
    };

struct usb_msosv1_descriptor {
    uint8_t *string;
    uint8_t  string_len;
    uint8_t  vendor_code;
    uint8_t *compat_id;
    uint16_t compat_id_len;
    uint8_t *comp_id_property;
    uint16_t comp_id_property_len;
};

/* MS OS 2.0 Header descriptor */
struct usb_msosv2_header_descriptor {
    uint32_t dwLength;
    uint16_t bcdVersion;
    uint16_t wIndex;
    uint8_t  bCount;
} __PACKED;

/*Microsoft OS 2.0 set header descriptor*/
struct usb_msosv2_set_header_descriptor {
    uint16_t wLength;
    uint16_t wDescriptorType;
    uint32_t dwWindowsVersion;
    uint16_t wDescriptorSetTotalLength;
} __PACKED;

/* Microsoft OS 2.0 compatibleID descriptor*/
struct usb_msosv2_comp_id_descriptor {
    uint16_t wLength;
    uint16_t wDescriptorType;
    uint8_t  compatibleID[8];
    uint8_t  subCompatibleID[8];
} __PACKED;

/* MS OS 2.0 property descriptor */
struct usb_msosv2_property_descriptor {
    uint16_t wLength;
    uint16_t wDescriptorType;
    uint32_t dwPropertyDataType;
    uint16_t wPropertyNameLength;
    const char *bPropertyName;
    uint32_t dwPropertyDataLength;
    const char *bPropertyData;
};

/* Microsoft OS 2.0 subset function descriptor  */
struct usb_msosv2_subset_function_descriptor {
    uint16_t wLength;
    uint16_t wDescriptorType;
    uint8_t  bFirstInterface;
    uint8_t  bReserved;
    uint16_t wSubsetLength;
} __PACKED;

struct usb_msosv2_descriptor {
    uint8_t *compat_id;
    uint16_t compat_id_len;
    uint8_t  vendor_code;
};

/* BOS header Descriptor */
struct usb_bos_header_descriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t wTotalLength;
    uint8_t  bNumDeviceCaps;
} __PACKED;

/* BOS Capability platform Descriptor */
struct usb_bos_capability_platform_descriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bDevCapabilityType;
    uint8_t  bReserved;
    uint8_t  PlatformCapabilityUUID[16];
} __PACKED;

/* BOS Capability MS OS Descriptors version 2 */
struct usb_bos_capability_msosv2_descriptor {
    uint32_t dwWindowsVersion;
    uint16_t wMSOSDescriptorSetTotalLength;
    uint8_t  bVendorCode;
    uint8_t  bAltEnumCode;
} __PACKED;

/* BOS Capability webusb */
struct usb_bos_capability_webusb_descriptor {
    uint16_t bcdVersion;
    uint8_t  bVendorCode;
    uint8_t  iLandingPage;
} __PACKED;

/* BOS Capability extension Descriptor*/
struct usb_bos_capability_extension_descriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bDevCapabilityType;
    uint32_t bmAttributes;
} __PACKED;

/* Microsoft OS 2.0 Platform Capability Descriptor */
struct usb_bos_capability_platform_msosv2_descriptor {
    struct usb_bos_capability_platform_descriptor platform_msos;
    struct usb_bos_capability_msosv2_descriptor data_msosv2;
} __PACKED;

/* WebUSB Platform Capability Descriptor */
struct usb_bos_capability_platform_webusb_descriptor {
    struct usb_bos_capability_platform_descriptor platform_webusb;
    struct usb_bos_capability_webusb_descriptor data_webusb;
} __PACKED;

struct usb_webusb_url_descriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bScheme;
    char     URL[];
} __PACKED;

struct usb_bos_descriptor {
    uint8_t *string;
    uint16_t string_len;
};

/* USB Device Capability Descriptor */
struct usb_device_capability_descriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bDevCapabilityType;
} __PACKED;


/* Macro for USB Device Descriptor */
/* same iSerial problem.
 * https://blog.csdn.net/asmxpl/article/details/21243913
 */
#define USB_DEVICE_DESCRIPTOR_INIT0(bcdUSB, bDeviceClass, bDeviceSubClass, bDeviceProtocol, idVendor, idProduct, bcdDevice, bNumConfigurations) \
    0x12,                                /* bLength */             \
    USB_DESC_TYPE_DEVICE,                /* bDescriptorType */     \
    WBVAL(bcdUSB),                       /* bcdUSB */              \
    bDeviceClass,                        /* bDeviceClass */        \
    bDeviceSubClass,                     /* bDeviceSubClass */     \
    bDeviceProtocol,                     /* bDeviceProtocol */     \
    USB_EP0_MPS,                         /* bMaxPacketSize */      \
    WBVAL(idVendor),                     /* idVendor */            \
    WBVAL(idProduct),                    /* idProduct */           \
    WBVAL(bcdDevice),                    /* bcdDevice */           \
    USB_STRING_MFC_INDEX,                /* iManufacturer */       \
    USB_STRING_PRODUCT_INDEX,            /* iProduct */            \
    USB_STRING_SERIAL_INDEX,             /* iSerial  */            \
    bNumConfigurations                   /* bNumConfigurations */  

// Device Init without 'iSerial'
#define USB_DEVICE_DESCRIPTOR_INIT(bcdUSB, bDeviceClass, bDeviceSubClass, bDeviceProtocol, idVendor, idProduct, bcdDevice, bNumConfigurations) \
    0x12,                                /* bLength */             \
    USB_DESC_TYPE_DEVICE,                /* bDescriptorType */     \
    WBVAL(bcdUSB),                       /* bcdUSB */              \
    bDeviceClass,                        /* bDeviceClass */        \
    bDeviceSubClass,                     /* bDeviceSubClass */     \
    bDeviceProtocol,                     /* bDeviceProtocol */     \
    USB_EP0_MPS,                         /* bMaxPacketSize */      \
    WBVAL(idVendor),                     /* idVendor */            \
    WBVAL(idProduct),                    /* idProduct */           \
    WBVAL(bcdDevice),                    /* bcdDevice */           \
    USB_STRING_MFC_INDEX,                /* iManufacturer */       \
    USB_STRING_PRODUCT_INDEX,            /* iProduct */            \
    0/*USB_STRING_SERIAL_INDEX*/,        /* iSerial (0 = None) */  \
    bNumConfigurations                   /* bNumConfigurations */  

/* Macro for USB Configuration Descriptor */
#define USB_CONFIG_DESCRIPTOR_INIT(wTotalLength, bNumInterfaces, bConfigurationValue, bmAttributes, bMaxPower) \
    0x09,                                /* bLength */             \
    USB_DESC_TYPE_CONFIGURATION,         /* bDescriptorType */     \
    WBVAL(wTotalLength),                 /* wTotalLength */        \
    bNumInterfaces,                      /* bNumInterfaces */      \
    bConfigurationValue,                 /* bConfigurationValue */ \
    0x00,                                /* iConfiguration */      \
    bmAttributes,                        /* bmAttributes */        \
    USB_CONFIG_POWER_MA(bMaxPower)       /* bMaxPower */           

/* Macro for USB Interface Descriptor */
#define USB_INTERFACE_DESCRIPTOR_INIT(bInterfaceNumber, bAlternateSetting, bNumEndpoints,                  \
                                      bInterfaceClass, bInterfaceSubClass, bInterfaceProtocol, iInterface) \
    0x09,                                /* bLength */             \
    USB_DESC_TYPE_INTERFACE,             /* bDescriptorType */     \
    bInterfaceNumber,                    /* bInterfaceNumber */    \
    bAlternateSetting,                   /* bAlternateSetting */   \
    bNumEndpoints,                       /* bNumEndpoints */       \
    bInterfaceClass,                     /* bInterfaceClass */     \
    bInterfaceSubClass,                  /* bInterfaceSubClass */  \
    bInterfaceProtocol,                  /* bInterfaceProtocol */  \
    iInterface                           /* iInterface */          

/* Macro for USB Endpoint Descriptor */
#define USB_EP_DESCRIPTOR_INIT(bEndpointAddress, bmAttributes, wMaxPacketSize, bInterval) \
    0x07,                                /* bLength */             \
    USB_DESC_TYPE_ENDPOINT,              /* bDescriptorType */     \
    bEndpointAddress,                    /* bEndpointAddress */    \
    bmAttributes,                        /* bmAttributes */        \
    WBVAL(wMaxPacketSize),               /* wMaxPacketSize */      \
    bInterval                            /* bInterval */

/* Macro for USB Interface Association Descriptor */
#define USB_IAD_INIT(bFirstInterface, bInterfaceCount, bFunctionClass, bFunctionSubClass, bFunctionProtocol) \
    0x08,                                /* bLength */             \
    USB_DESC_TYPE_INTERFACE_ASSOCIATION, /* bDescriptorType */     \
    bFirstInterface,                     /* bFirstInterface */     \
    bInterfaceCount,                     /* bInterfaceCount */     \
    bFunctionClass,                      /* bFunctionClass */      \
    bFunctionSubClass,                   /* bFunctionSubClass */   \
    bFunctionProtocol,                   /* bFunctionProtocol */   \
    0x00                                 /* iFunction */

/* Macro for USB LangID Descriptor */
#define USB_LANGID_INIT(id)                                        \
    0x04,                                /* bLength */             \
    USB_DESC_TYPE_STRING,                /* bDescriptorType */     \
    WBVAL(id)                            /* wLangID0 */

/* Macro for USB Device Qualifier Descriptor(usb2.0) */
#define USB_QUALIFIER_INIT(bNumConfigurations)                     \
    0x0A,                                /* bLength */             \
    USB_DESC_TYPE_DEVICE_QUALIFIER,      /* bDescriptorType */     \
    0x00, 0x02,                          /* bcdUSB */              \
    0x00,                                /* bDeviceClass */        \
    0x00,                                /* bDeviceSubClass */     \
    0x00,                                /* bDeviceProtocol */     \
    USB_EP0_MPS,                         /* bMaxPacketSize */      \
    bNumConfigurations,                  /* bNumConfigurations */  \
    0                                    /* bReserved */


#endif // _USB_DEF_H
