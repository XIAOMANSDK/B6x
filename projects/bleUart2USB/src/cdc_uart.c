#include "usbd.h"
#include "usbd_cdc.h"
#include "uart.h"
#include "app.h"
#include "prf_sess.h"

#define USBD_BCD                  USB_2_0 // Version
#define USBD_VID                  0xFFFF  // Vendor ID
#define USBD_PID                  0xFFFF  // Product ID
#define USBD_MAX_POWER            100     // unit in mA
#define USBD_LANGID_STRING        0x0409  // English(US)


#define ENB_CDC_CNT               1

#if (ENB_CDC_CNT == 0 || ENB_CDC_CNT > 3)
#error "The count of USB-Serial be 1 ~ 3."
#endif


/*
 * Descriptor
 ****************************************************************************
 */

#define CDC0_IN_EP                0x81
#define CDC0_OUT_EP               0x01
#define CDC0_INT_EP               0x81+ENB_CDC_CNT
#define CDC0_INTF_NUM             0

#if (ENB_CDC_CNT > 1)
#define CDC1_IN_EP                0x82
#define CDC1_OUT_EP               0x02
#define CDC1_INT_EP               0x82+ENB_CDC_CNT
#define CDC1_INTF_NUM             2
#endif

#if (ENB_CDC_CNT > 2)
#define CDC2_IN_EP                0x83
#define CDC2_OUT_EP               0x03
#define CDC2_INT_EP               0x83+ENB_CDC_CNT
#define CDC2_INTF_NUM             4
#endif

#define USB_CDC_INTF_CNT          (2 * ENB_CDC_CNT) // Cmd Intf + Data Intf
#define USB_CDC_INTF_END          (USB_CDC_INTF_CNT - 1)
#define USB_CDC_CONFIG_SIZE       (9 + CDC_ACM_DESCRIPTOR_LEN * ENB_CDC_CNT)

/*!< cdc device descriptor */
static const uint8_t cdc_descriptor[] = {
    /* Descriptor - Device (Size:18) */
    USB_DEVICE_DESCRIPTOR_INIT(USBD_BCD, 0xEF, 0x02, 0x01, USBD_VID, USBD_PID, 0x0100, 0x01),
    
    /* Descriptor - Configuration (Total Size:9+Intf_Size) */
    USB_CONFIG_DESCRIPTOR_INIT(USB_CDC_CONFIG_SIZE, USB_CDC_INTF_CNT, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),

    /* Descriptor - CDC Interface (Size:66) */
    CDC_ACM_DESCRIPTOR_INIT(0x00, CDC0_INT_EP, CDC0_OUT_EP, CDC0_IN_EP, 0x02),
    #if (ENB_CDC_CNT > 1)
    CDC_ACM_DESCRIPTOR_INIT(0x02, CDC1_INT_EP, CDC1_OUT_EP, CDC1_IN_EP, 0x02),
    #endif
    #if (ENB_CDC_CNT > 2)
    CDC_ACM_DESCRIPTOR_INIT(0x04, CDC2_INT_EP, CDC2_OUT_EP, CDC2_IN_EP, 0x02),
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
    WCHAR('-'),
    WCHAR('S'),
    WCHAR('e'),
    WCHAR('r'),
    WCHAR('i'),
    WCHAR('a'),
    WCHAR('l'),
    
    // String3 - iSerialNumber
    0x10,                       /* bLength */
    USB_DESC_TYPE_STRING,       /* bDescriptorType */
    WCHAR('6'),
    WCHAR('.'),
    WCHAR('2'),
    WCHAR('2'),
    WCHAR('.'),
    WCHAR('0'),
    WCHAR('7'),
    
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

static const usbd_class_t class_tab[] = {
    USBD_CLASS_T(0, USB_CDC_INTF_END, &usbd_cdc_class_handler),
};

static const usbd_config_t cdc_configuration[] = {
    USBD_CONFIG_T(1, USB_CDC_INTF_CNT, class_tab, endpoint_tab)
};


/*
 * Handlers
 ****************************************************************************
 */

volatile uint8_t dtr_enable = 0;

void usbd_cdc_updated(usbd_cdc_t *cdc, uint8_t type)
{
    if (type == CDC_LINE_STATE) {
        USB_LOG_RAW("CDC Update(ep:0x%02X,<DTR:0x%x,RTS:0x%x>)\r\n", cdc->ep_in, (cdc->line_state & 0x01), (cdc->line_state & 0x02));

        dtr_enable = cdc->line_state;
    } else {
        USB_LOG_RAW("CDC Update(ep:0x%02X,<Rate:%d,DataBits:%d,Parity:%d,StopBits:%d>)\r\n", cdc->ep_in, 
                            cdc->line_coding.dwDTERate, cdc->line_coding.bDataBits,
                            cdc->line_coding.bParityType, cdc->line_coding.bCharFormat);
    }
}

void usbd_cdc_bulk_out_handler(uint8_t ep)
{
    uint16_t read_byte;
    uint8_t data[CDC_BULK_EP_MPS];
    
    read_byte = usbd_ep_read(ep, CDC_BULK_EP_MPS, data);
    USB_LOG_RAW("CDC Bulk Out(ep:%d,len:%d)\r\n", ep, read_byte);
    
    /*!< here you can output data to hardware */
    //for (uint8_t i = 0; i < read_byte; i++) {
    //    uart_putc(0, data[i]);
    //}
    if (app_state_get() > APP_READY)
    {
        sess_txd_send(app_env.curidx, read_byte, data);
    }
    else
    {
        uart_send(UART1_PORT, read_byte, data);
    }
}

__USBIRQ void usbd_notify_handler(uint8_t event, void *arg)
{
    switch (event) {
        case USBD_EVENT_RESET:
            usbd_cdc_reset();
            break;

        default:
            break;
    }
}


/*
 * Test Functions
 ****************************************************************************
 */

#define CDC_BULK_TX_SIZE  (CDC_BULK_EP_MPS-1)

uint8_t cdc_bulk_buff[CDC_BULK_TX_SIZE];

void usbdInit(void)
{
    usbd_init();
    usbd_register(cdc_descriptor, cdc_configuration);
    
    usbd_cdc_init(0, CDC0_INTF_NUM, CDC0_IN_EP);
    #if (ENB_CDC_CNT > 1)
    usbd_cdc_init(1, CDC1_INTF_NUM, CDC1_IN_EP);
    #endif
    #if (ENB_CDC_CNT > 2)
    usbd_cdc_init(2, CDC2_INTF_NUM, CDC2_IN_EP);
    #endif
    
    for (uint8_t i = 0; i < CDC_BULK_TX_SIZE; i++) {
        cdc_bulk_buff[i] = '0' + i;
    }
}

void usbdTest(void)
{
    if (!usbd_is_configured())
        return;
   
    if (dtr_enable)
    {
        uint8_t status = usbd_cdc_ep_send(CDC0_IN_EP, CDC_BULK_TX_SIZE, cdc_bulk_buff);
        USB_LOG_RAW("CDC0 Send(sta:%d,len:%d)\r\n", status, CDC_BULK_TX_SIZE);
        #if (ENB_CDC_CNT > 1)
        status = usbd_cdc_ep_send(CDC1_IN_EP, CDC_BULK_TX_SIZE, cdc_bulk_buff);
        USB_LOG_RAW("CDC1 Send(sta:%d,len:%d)\r\n", status, CDC_BULK_TX_SIZE);
        #endif
        #if (ENB_CDC_CNT > 2)
        status = usbd_cdc_ep_send(CDC2_IN_EP, CDC_BULK_TX_SIZE, cdc_bulk_buff);
        USB_LOG_RAW("CDC2 Send(sta:%d,len:%d)\r\n", status, CDC_BULK_TX_SIZE);
        #endif
    }
}
