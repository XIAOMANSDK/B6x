/**
 ******************************************************************************
 * @file    cdc_uart.c
 * @brief   USB转UART桥接功能实现
 *
 * 本文件实现USB CDC类设备功能，通过USB接口模拟虚拟串口，实现USB与UART之间的数据转换。
 * 支持多路CDC串口通道，可根据配置启用1~3个虚拟串口。
 * 本代码实现了一个USB转UART（CDC类）设备功能，主要工作原理如下：

 * USB设备初始化：配置USB设备描述符、端点描述符和类描述符，建立USB通信基础
 * CDC类功能实现：通过USB CDC（Communications Device Class）协议模拟串口设备
 * 数据双向传输：
 * USB主机发送数据 → 通过Bulk OUT端点接收 → UART发送数据
 * UART接收数据 → 通过Bulk IN端点发送 → USB主机接收数据
 * 流控信号处理：处理DTR/RTS等串口控制信号，实现虚拟串口的完整功能
 * 整个系统实现了USB接口与UART接口之间的透明桥接，允许PC通过USB接口与UART设备进行通信。
 ******************************************************************************
 */

#include "drvs.h"
#include "usbd.h"
#include "usbd_cdc.h"

/**
 * @defgroup USB_Config USB配置参数
 * @{
 */
#define USBD_BCD                  USB_2_0     ///< USB规范版本：USB 2.0
#define USBD_VID                  0xFFFF      ///< 厂商ID
#define USBD_PID                  0xFFFF      ///< 产品ID
#define USBD_MAX_POWER            100         ///< 最大功耗：100mA
#define USBD_LANGID_STRING        0x0409      ///< 字符串语言ID：英语(美国)
/** @} */

/**
 * @defgroup CDC_Config CDC串口配置
 * @{
 */
#define ENB_CDC_CNT               1           ///< 启用的CDC串口数量

#if (ENB_CDC_CNT == 0 || ENB_CDC_CNT > 3)
#error "The count of USB-Serial be 1 ~ 3."    ///< CDC串口数量必须在1~3之间
#endif
/** @} */

/*
 * 描述符定义
 ******************************************************************************
 */

/**
 * @defgroup Endpoint_Def 端点定义
 * @brief 定义CDC串口使用的USB端点地址
 * @{
 */
#define CDC0_IN_EP                0x81        ///< CDC0数据输入端点
#define CDC0_OUT_EP               0x01        ///< CDC0数据输出端点
#define CDC0_INT_EP               0x81+ENB_CDC_CNT  ///< CDC0中断端点
#define CDC0_INTF_NUM             0           ///< CDC0接口编号

#if (ENB_CDC_CNT > 1)
#define CDC1_IN_EP                0x82        ///< CDC1数据输入端点
#define CDC1_OUT_EP               0x02        ///< CDC1数据输出端点
#define CDC1_INT_EP               0x82+ENB_CDC_CNT  ///< CDC1中断端点
#define CDC1_INTF_NUM             2           ///< CDC1接口编号
#endif

#if (ENB_CDC_CNT > 2)
#define CDC2_IN_EP                0x83        ///< CDC2数据输入端点
#define CDC2_OUT_EP               0x03        ///< CDC2数据输出端点
#define CDC2_INT_EP               0x83+ENB_CDC_CNT  ///< CDC2中断端点
#define CDC2_INTF_NUM             4           ///< CDC2接口编号
#endif

#define USB_CDC_INTF_CNT          (2 * ENB_CDC_CNT)  ///< CDC接口总数（每个CDC包含命令+数据接口）
#define USB_CDC_INTF_END          (USB_CDC_INTF_CNT - 1)  ///< 最后一个接口编号
#define USB_CDC_CONFIG_SIZE       (9 + CDC_ACM_DESCRIPTOR_LEN * ENB_CDC_CNT)  ///< 配置描述符总大小
/** @} */

/**
 * @brief CDC设备描述符
 * 
 * 包含设备描述符、配置描述符、接口描述符、端点描述符和字符串描述符
 */
static const uint8_t cdc_descriptor[] = {
    /* 设备描述符 (大小:18字节) */
    USB_DEVICE_DESCRIPTOR_INIT(USBD_BCD, 0xEF, 0x02, 0x01, USBD_VID, USBD_PID, 0x0100, 0x01),
    
    /* 配置描述符 (总大小:9+接口大小) */
    USB_CONFIG_DESCRIPTOR_INIT(USB_CDC_CONFIG_SIZE, USB_CDC_INTF_CNT, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),

    /* CDC接口描述符 (大小:66字节) */
    CDC_ACM_DESCRIPTOR_INIT(0x00, CDC0_INT_EP, CDC0_OUT_EP, CDC0_IN_EP, 0x02),  ///< CDC0接口描述符
    #if (ENB_CDC_CNT > 1)
    CDC_ACM_DESCRIPTOR_INIT(0x02, CDC1_INT_EP, CDC1_OUT_EP, CDC1_IN_EP, 0x02),  ///< CDC1接口描述符
    #endif
    #if (ENB_CDC_CNT > 2)
    CDC_ACM_DESCRIPTOR_INIT(0x04, CDC2_INT_EP, CDC2_OUT_EP, CDC2_IN_EP, 0x02),  ///< CDC2接口描述符
    #endif

    /* 字符串描述符 */
    // String0 - 语言ID (大小:4字节)
    USB_LANGID_INIT(USBD_LANGID_STRING),
    
    // String1 - 厂商字符串
    0x02,                       ///< bLength - 描述符长度
    USB_DESC_TYPE_STRING,       ///< bDescriptorType - 字符串描述符类型

    // String2 - 产品字符串
    0x16,                       ///< bLength - 描述符长度
    USB_DESC_TYPE_STRING,       ///< bDescriptorType - 字符串描述符类型
    WCHAR('U'),                 ///< 产品名称：USB-Serial
    WCHAR('S'),
    WCHAR('B'),
    WCHAR('-'),
    WCHAR('S'),
    WCHAR('e'),
    WCHAR('r'),
    WCHAR('i'),
    WCHAR('a'),
    WCHAR('l'),
    
    // String3 - 序列号字符串（注释掉）
//    0x10,                       /* bLength */
//    USB_DESC_TYPE_STRING,       /* bDescriptorType */
//    WCHAR('6'),
//    WCHAR('.'),
//    WCHAR('2'),
//    WCHAR('2'),
//    WCHAR('.'),
//    WCHAR('0'),
//    WCHAR('7'),
    
    /* 设备限定描述符 (大小:10字节) */
    #if (USBD_BCD == USB_2_0)
    USB_QUALIFIER_INIT(0x01),   ///< USB 2.0设备限定描述符
    #endif
    
    /* 描述符结束标记 */
    0x00                        ///< 描述符结束
};

/*
 * USB配置结构
 ******************************************************************************
 */

/**
 * @brief USB端点配置表
 * 
 * 配置所有CDC串口使用的端点类型、大小和回调函数
 */
static const usbd_ep_t endpoint_tab[] = {
    USBD_EP_T(CDC0_INT_EP, USB_EP_TYPE_INTERRUPT, CDC_INT_EP_MPS, NULL),                    ///< CDC0中断端点
    USBD_EP_T(CDC0_OUT_EP, USB_EP_TYPE_BULK,      CDC_BULK_EP_MPS, &usbd_cdc_bulk_out_handler),  ///< CDC0数据输出端点
    USBD_EP_T(CDC0_IN_EP,  USB_EP_TYPE_BULK,      CDC_BULK_EP_MPS, &usbd_cdc_bulk_in_handler),   ///< CDC0数据输入端点
    #if (ENB_CDC_CNT > 1)
    USBD_EP_T(CDC1_INT_EP, USB_EP_TYPE_INTERRUPT, CDC_INT_EP_MPS, NULL),                    ///< CDC1中断端点
    USBD_EP_T(CDC1_OUT_EP, USB_EP_TYPE_BULK,      CDC_BULK_EP_MPS, &usbd_cdc_bulk_out_handler),  ///< CDC1数据输出端点
    USBD_EP_T(CDC1_IN_EP,  USB_EP_TYPE_BULK,      CDC_BULK_EP_MPS, &usbd_cdc_bulk_in_handler),   ///< CDC1数据输入端点
    #endif
    #if (ENB_CDC_CNT > 2)
    USBD_EP_T(CDC2_INT_EP, USB_EP_TYPE_INTERRUPT, CDC_INT_EP_MPS, NULL),                    ///< CDC2中断端点
    USBD_EP_T(CDC2_OUT_EP, USB_EP_TYPE_BULK,      CDC_BULK_EP_MPS, &usbd_cdc_bulk_out_handler),  ///< CDC2数据输出端点
    USBD_EP_T(CDC2_IN_EP,  USB_EP_TYPE_BULK,      CDC_BULK_EP_MPS, &usbd_cdc_bulk_in_handler),   ///< CDC2数据输入端点
    #endif
};

/**
 * @brief USB类配置表
 * 
 * 配置CDC类处理函数
 */
static const usbd_class_t class_tab[] = {
    USBD_CLASS_T(0, USB_CDC_INTF_END, &usbd_cdc_class_handler),  ///< CDC类处理配置
};

/**
 * @brief USB设备配置表
 */
static const usbd_config_t cdc_configuration[] = {
    USBD_CONFIG_T(1, USB_CDC_INTF_CNT, class_tab, endpoint_tab)  ///< USB配置：1个配置，多个接口
};

/*
 * 事件处理函数
 ******************************************************************************
 */

volatile uint8_t dtr_enable = 0;  ///< DTR使能标志，用于流控

/**
 ****************************************************************************************
 * @brief CDC状态更新回调函数
 *
 * 当CDC串口状态发生变化时被调用，包括线路状态和线路编码参数的变化
 *
 * @param[in] cdc  CDC设备指针
 * @param[in] type 更新类型：CDC_LINE_STATE或CDC_LINE_CODING
 ****************************************************************************************
 */
void usbd_cdc_updated(usbd_cdc_t *cdc, uint8_t type)
{
    if (type == CDC_LINE_STATE) {
        /* 线路状态更新：DTR/RTS信号变化 */
        USB_LOG_RAW("CDC Update(ep:0x%02X,<DTR:0x%x,RTS:0x%x>)\r\n", cdc->ep_in, (cdc->line_state & 0x01), (cdc->line_state & 0x02));

        dtr_enable = cdc->line_state;  ///< 更新DTR使能状态
    } else {
        /* 线路编码参数更新：波特率、数据位等 */
        USB_LOG_RAW("CDC Update(ep:0x%02X,<Rate:%d,DataBits:%d,Parity:%d,StopBits:%d>)\r\n", cdc->ep_in, 
                            cdc->line_coding.dwDTERate, cdc->line_coding.bDataBits,
                            cdc->line_coding.bParityType, cdc->line_coding.bCharFormat);
    }
}

/**
 ****************************************************************************************
 * @brief CDC Bulk OUT端点数据处理函数
 *
 * 当USB主机发送数据到Bulk OUT端点时被调用，将数据通过UART发送出去
 *
 * @param[in] ep 端点地址
 ****************************************************************************************
 */
void usbd_cdc_bulk_out_handler(uint8_t ep)
{
    uint16_t read_byte;
    uint8_t data[CDC_BULK_EP_MPS];
    
    /* 从USB端点读取数据 */
    read_byte = usbd_ep_read(ep, CDC_BULK_EP_MPS, data);
    USB_LOG_RAW("CDC Bulk Out(ep:%d,len:%d)\r\n", ep, read_byte);
    
    /* 通过UART发送数据到硬件 */
    for (uint8_t i = 0; i < read_byte; i++) {
        uart_putc(UART1_PORT, data[i]);  ///< 通过UART1发送数据
    }
}

/**
 ****************************************************************************************
 * @brief USB设备事件通知处理函数
 *
 * 处理USB设备的各种事件，如复位、挂起、恢复等
 *
 * @param[in] event 事件类型
 * @param[in] arg   事件参数
 ****************************************************************************************
 */
__USBIRQ void usbd_notify_handler(uint8_t event, void *arg)
{
    switch (event) {
        case USBD_EVENT_RESET:
            /* USB复位事件：重置CDC状态 */
            usbd_cdc_reset();
            break;

        default:
            break;
    }
}

/*
 * 测试功能
 ******************************************************************************
 */

#define CDC_BULK_TX_SIZE  (CDC_BULK_EP_MPS-1)  ///< 批量传输数据大小

uint8_t cdc_bulk_buff[CDC_BULK_TX_SIZE];  ///< 批量传输数据缓冲区

/**
 ****************************************************************************************
 * @brief USB设备初始化函数
 *
 * 初始化USB设备，包括时钟、描述符、CDC类等
 ****************************************************************************************
 */
void usbdInit(void)
{
    /* 使能USB时钟和IO引脚 */
    rcc_usb_en();  ///< 使能USB时钟
    usbd_init();   ///< USB设备初始化
    
    /* 注册USB描述符和配置 */
    usbd_register(cdc_descriptor, cdc_configuration);
    
    /* 初始化CDC串口 */
    usbd_cdc_init(0, CDC0_INTF_NUM, CDC0_IN_EP);  ///< 初始化CDC0
    NVIC_EnableIRQ(USB_IRQn);  ///< 使能USB中断
    #if (ENB_CDC_CNT > 1)
    usbd_cdc_init(1, CDC1_INTF_NUM, CDC1_IN_EP);  ///< 初始化CDC1
    #endif
    #if (ENB_CDC_CNT > 2)
    usbd_cdc_init(2, CDC2_INTF_NUM, CDC2_IN_EP);  ///< 初始化CDC2
    #endif
    
    /* 初始化测试数据缓冲区 */
    for (uint8_t i = 0; i < CDC_BULK_TX_SIZE; i++) {
        cdc_bulk_buff[i] = '0' + i;  ///< 填充测试数据：'0','1','2'...
    }
}

/**
 ****************************************************************************************
 * @brief USB设备测试函数
 *
 * 在DTR信号有效时，通过所有启用的CDC串口发送测试数据
 ****************************************************************************************
 */
void usbdTest(void)
{
    /* 检查USB设备是否已配置 */
    if (!usbd_is_configured())
        return;
   
    /* 检查DTR信号是否有效（主机已准备好接收数据） */
    if (dtr_enable)
    {
        uint8_t status;
        
        /* 通过CDC0发送测试数据 */
        status = usbd_cdc_ep_send(CDC0_IN_EP, CDC_BULK_TX_SIZE, cdc_bulk_buff);
        USB_LOG_RAW("CDC0 Send(sta:%d,len:%d)\r\n", status, CDC_BULK_TX_SIZE);
        
        #if (ENB_CDC_CNT > 1)
        /* 通过CDC1发送测试数据 */
        status = usbd_cdc_ep_send(CDC1_IN_EP, CDC_BULK_TX_SIZE, cdc_bulk_buff);
        USB_LOG_RAW("CDC1 Send(sta:%d,len:%d)\r\n", status, CDC_BULK_TX_SIZE);
        #endif
        
        #if (ENB_CDC_CNT > 2)
        /* 通过CDC2发送测试数据 */
        status = usbd_cdc_ep_send(CDC2_IN_EP, CDC_BULK_TX_SIZE, cdc_bulk_buff);
        USB_LOG_RAW("CDC2 Send(sta:%d,len:%d)\r\n", status, CDC_BULK_TX_SIZE);
        #endif
    }
}
