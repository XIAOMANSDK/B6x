/**
 ****************************************************************************************
 *
 * @file cdc_uart.c
 *
 * @brief USB CDC virtual serial port (USB-to-UART bridge)
 *
 * @details
 * Implements a USB CDC class device that bridges USB virtual COM port to hardware UART.
 * Supports 1~3 CDC channels depending on ENB_CDC_CNT configuration.
 *
 * Data flow:
 *   USB host -> Bulk OUT endpoint -> UART TX
 *   UART RX  -> Bulk IN endpoint  -> USB host
 *
 * Flow control: DTR/RTS signals are tracked for virtual COM port open/close detection.
 *
 ****************************************************************************************
 */

#include "drvs.h"
#include "usbd.h"
#include "usbd_cdc.h"
#include "cdc_uart.h"

/*
 * DEFINES - USB Device Configuration
 ****************************************************************************************
 */

/// USB specification version
#define USBD_BCD                  USB_2_0

/// Vendor ID
#define USBD_VID                  0xFFFF

/// Product ID
#define USBD_PID                  0xFFFF

/// Max power consumption (mA)
#define USBD_MAX_POWER            100

/// String descriptor language ID (English US)
#define USBD_LANGID_STRING        0x0409

/*
 * DEFINES - CDC Port Configuration
 ****************************************************************************************
 */

/// Number of active CDC serial ports (1~3)
#define ENB_CDC_CNT               1

#if (ENB_CDC_CNT == 0 || ENB_CDC_CNT > 3)
#error "ENB_CDC_CNT must be 1, 2, or 3"
#endif

/*
 * DEFINES - Endpoint Addresses
 ****************************************************************************************
 */

#define CDC0_IN_EP                0x81        ///< CDC0 data IN endpoint
#define CDC0_OUT_EP               0x01        ///< CDC0 data OUT endpoint
#define CDC0_INT_EP               (0x81 + ENB_CDC_CNT)  ///< CDC0 interrupt endpoint
#define CDC0_INTF_NUM             0           ///< CDC0 interface number

#if (ENB_CDC_CNT > 1)
#define CDC1_IN_EP                0x82        ///< CDC1 data IN endpoint
#define CDC1_OUT_EP               0x02        ///< CDC1 data OUT endpoint
#define CDC1_INT_EP               (0x82 + ENB_CDC_CNT)  ///< CDC1 interrupt endpoint
#define CDC1_INTF_NUM             2           ///< CDC1 interface number
#endif

#if (ENB_CDC_CNT > 2)
#define CDC2_IN_EP                0x83        ///< CDC2 data IN endpoint
#define CDC2_OUT_EP               0x03        ///< CDC2 data OUT endpoint
#define CDC2_INT_EP               (0x83 + ENB_CDC_CNT)  ///< CDC2 interrupt endpoint
#define CDC2_INTF_NUM             4           ///< CDC2 interface number
#endif

/// Total CDC interfaces (each CDC = command + data interface)
#define USB_CDC_INTF_CNT          (2 * ENB_CDC_CNT)

/// Last interface number
#define USB_CDC_INTF_END          (USB_CDC_INTF_CNT - 1)

/// Configuration descriptor total size
#define USB_CDC_CONFIG_SIZE       (9 + CDC_ACM_DESCRIPTOR_LEN * ENB_CDC_CNT)

/*
 * LOCAL DATA - Descriptors
 ****************************************************************************************
 */

/// CDC device, configuration, interface, endpoint, and string descriptors
static const uint8_t cdc_descriptor[] = {
    /* Device descriptor (18 bytes) */
    USB_DEVICE_DESCRIPTOR_INIT(USBD_BCD, 0xEF, 0x02, 0x01, USBD_VID, USBD_PID, 0x0100, 0x01),

    /* Configuration descriptor */
    USB_CONFIG_DESCRIPTOR_INIT(USB_CDC_CONFIG_SIZE, USB_CDC_INTF_CNT, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),

    /* CDC interface descriptors (66 bytes each) */
    CDC_ACM_DESCRIPTOR_INIT(0x00, CDC0_INT_EP, CDC0_OUT_EP, CDC0_IN_EP, 0x02),
#if (ENB_CDC_CNT > 1)
    CDC_ACM_DESCRIPTOR_INIT(0x02, CDC1_INT_EP, CDC1_OUT_EP, CDC1_IN_EP, 0x02),
#endif
#if (ENB_CDC_CNT > 2)
    CDC_ACM_DESCRIPTOR_INIT(0x04, CDC2_INT_EP, CDC2_OUT_EP, CDC2_IN_EP, 0x02),
#endif

    /* String descriptors */
    USB_LANGID_INIT(USBD_LANGID_STRING),

    /* String 1 - Manufacturer (empty) */
    0x02,
    USB_DESC_TYPE_STRING,

    /* String 2 - Product: "USB-Serial" */
    0x16,
    USB_DESC_TYPE_STRING,
    WCHAR('U'), WCHAR('S'), WCHAR('B'), WCHAR('-'),
    WCHAR('S'), WCHAR('e'), WCHAR('r'), WCHAR('i'), WCHAR('a'), WCHAR('l'),

    /* Device qualifier descriptor (USB 2.0 only) */
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

/// Endpoint configuration table
static const usbd_ep_t endpoint_tab[] = {
    USBD_EP_T(CDC0_INT_EP, USB_EP_TYPE_INTERRUPT, CDC_INT_EP_MPS, NULL),
    USBD_EP_T(CDC0_OUT_EP, USB_EP_TYPE_BULK,      CDC_BULK_EP_MPS, &usbd_cdc_bulk_out_handler),
    USBD_EP_T(CDC0_IN_EP,  USB_EP_TYPE_BULK,      CDC_BULK_EP_MPS, &usbd_cdc_bulk_in_handler),
#if (ENB_CDC_CNT > 1)
    USBD_EP_T(CDC1_INT_EP, USB_EP_TYPE_INTERRUPT, CDC_INT_EP_MPS, NULL),
    USBD_EP_T(CDC1_OUT_EP, USB_EP_TYPE_BULK,      CDC_BULK_EP_MPS, &usbd_cdc_bulk_out_handler),
    USBD_EP_T(CDC1_IN_EP,  USB_EP_TYPE_BULK,      CDC_BULK_EP_MPS, &usbd_cdc_bulk_in_handler),
#endif
#if (ENB_CDC_CNT > 2)
    USBD_EP_T(CDC2_INT_EP, USB_EP_TYPE_INTERRUPT, CDC_INT_EP_MPS, NULL),
    USBD_EP_T(CDC2_OUT_EP, USB_EP_TYPE_BULK,      CDC_BULK_EP_MPS, &usbd_cdc_bulk_out_handler),
    USBD_EP_T(CDC2_IN_EP,  USB_EP_TYPE_BULK,      CDC_BULK_EP_MPS, &usbd_cdc_bulk_in_handler),
#endif
};

/// Class handler table
static const usbd_class_t class_tab[] = {
    USBD_CLASS_T(0, USB_CDC_INTF_END, &usbd_cdc_class_handler),
};

/// Device configuration table
static const usbd_config_t cdc_configuration[] = {
    USBD_CONFIG_T(1, USB_CDC_INTF_CNT, class_tab, endpoint_tab)
};

/*
 * LOCAL DATA
 ****************************************************************************************
 */

/// DTR enable flag (host has opened the virtual COM port)
static volatile uint8_t dtr_enable = 0;

/// Bulk transfer test buffer
#define CDC_BULK_TX_SIZE  (CDC_BULK_EP_MPS - 1)

static uint8_t cdc_bulk_buff[CDC_BULK_TX_SIZE];

/*
 * CALLBACK FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief CDC status update callback (line state / line coding change)
 *
 * @param[in] cdc  CDC device pointer
 * @param[in] type Update type: CDC_LINE_STATE or CDC_LINE_CODING
 ****************************************************************************************
 */
void usbd_cdc_updated(usbd_cdc_t *cdc, uint8_t type)
{
    if (type == CDC_LINE_STATE)
    {
        USB_LOG_RAW("CDC Update(ep:0x%02X, DTR:%d, RTS:%d)\r\n",
                    cdc->ep_in, (cdc->line_state & 0x01), (cdc->line_state & 0x02));
        dtr_enable = cdc->line_state;
    }
    else
    {
        USB_LOG_RAW("CDC Update(ep:0x%02X, Rate:%d, DataBits:%d, Parity:%d, StopBits:%d)\r\n",
                    cdc->ep_in,
                    (int)cdc->line_coding.dwDTERate, cdc->line_coding.bDataBits,
                    cdc->line_coding.bParityType, cdc->line_coding.bCharFormat);
    }
}

/**
 ****************************************************************************************
 * @brief CDC Bulk OUT endpoint handler (USB host -> UART)
 *
 * @param[in] ep  Endpoint address
 ****************************************************************************************
 */
void usbd_cdc_bulk_out_handler(uint8_t ep)
{
    uint16_t read_byte;
    uint8_t data[CDC_BULK_EP_MPS];

    read_byte = usbd_ep_read(ep, CDC_BULK_EP_MPS, data);
    USB_LOG_RAW("CDC Bulk Out(ep:%d, len:%d)\r\n", ep, read_byte);

    /* Forward USB data to UART1 */
    for (uint16_t i = 0; i < read_byte; i++)
    {
        uart_putc(UART1_PORT, data[i]);
    }
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
            usbd_cdc_reset();
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
 * @brief Initialize USB device with CDC class
 *
 * @details
 * - Enable USB clock and IO pins
 * - Register descriptors and configuration
 * - Initialize all CDC ports
 * - Enable USB interrupt
 * - Prepare test data buffer
 ****************************************************************************************
 */
void usbdInit(void)
{
    rcc_usb_en();
    usbd_init();

    usbd_register(cdc_descriptor, cdc_configuration);

    usbd_cdc_init(0, CDC0_INTF_NUM, CDC0_IN_EP);
#if (ENB_CDC_CNT > 1)
    usbd_cdc_init(1, CDC1_INTF_NUM, CDC1_IN_EP);
#endif
#if (ENB_CDC_CNT > 2)
    usbd_cdc_init(2, CDC2_INTF_NUM, CDC2_IN_EP);
#endif

    NVIC_EnableIRQ(USB_IRQn);

    /* Fill test data buffer with '0'..'9' pattern */
    for (uint16_t i = 0; i < CDC_BULK_TX_SIZE; i++)
    {
        cdc_bulk_buff[i] = '0' + (i % 10);
    }
}

/**
 ****************************************************************************************
 * @brief USB CDC test function (call in main loop)
 *
 * @details
 * When DTR is active (host opened the COM port), sends a test pattern
 * through all enabled CDC ports.
 ****************************************************************************************
 */
void usbdTest(void)
{
    if (!usbd_is_configured())
        return;

    if (dtr_enable)
    {
        uint8_t status;

        status = usbd_cdc_ep_send(CDC0_IN_EP, CDC_BULK_TX_SIZE, cdc_bulk_buff);
        USB_LOG_RAW("CDC0 Send(sta:%d, len:%d)\r\n", status, CDC_BULK_TX_SIZE);

#if (ENB_CDC_CNT > 1)
        status = usbd_cdc_ep_send(CDC1_IN_EP, CDC_BULK_TX_SIZE, cdc_bulk_buff);
        USB_LOG_RAW("CDC1 Send(sta:%d, len:%d)\r\n", status, CDC_BULK_TX_SIZE);
#endif

#if (ENB_CDC_CNT > 2)
        status = usbd_cdc_ep_send(CDC2_IN_EP, CDC_BULK_TX_SIZE, cdc_bulk_buff);
        USB_LOG_RAW("CDC2 Send(sta:%d, len:%d)\r\n", status, CDC_BULK_TX_SIZE);
#endif
    }
}
