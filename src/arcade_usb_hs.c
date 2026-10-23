/*
 * arcade_usb_hs.c
 *
 *  Created on: 2026年9月10日
 *      Author: Admin
 */

#include "arcade_usb_hs.h"

#include "arcade_usb_mux.h"
#if ARCADE_USB_HS_MODE

#include <errno.h>
#include "CH58x_common.h"

/* General */
#define pUSBHS_SetupReqPak            ((PUSB_SETUP_REQ) USBHS_EP0_Buf)

#define DEF_UEP_IN                    0x80
#define DEF_UEP_OUT                   0x00
/* Endpoint Number */
#define DEF_UEP_BUSY                  0x01
#define DEF_UEP_FREE                  0x00
#define DEF_UEP_NUM                   16
#define DEF_UEP0                      0x00
#define DEF_UEP1                      0x01
#define DEF_UEP2                      0x02
#define DEF_UEP3                      0x03
#define DEF_UEP4                      0x04
#define DEF_UEP5                      0x05
#define DEF_UEP6                      0x06
#define DEF_UEP7                      0x07
#define DEF_UEP8                      0x08
#define DEF_UEP9                      0x09
#define DEF_UEP10                     0x0A
#define DEF_UEP11                     0x0B
#define DEF_UEP12                     0x0C
#define DEF_UEP13                     0x0D
#define DEF_UEP14                     0x0E
#define DEF_UEP15                     0x0F

#define TEST_ENABLE                   0x01
#define TEST_MASK                     0x0F

#define USBHSD_UEP_RXDMA_BASE         0x40009024
#define USBHSD_UEP_TXDMA_BASE         0x40009040
#define USBHSD_UEP_TXLEN_BASE         0x404090A0
#define USBHSD_UEP_TXCTL_BASE         0x404090A2
#define USBHSD_UEP_TX_EN(N)           ((uint16_t) (0x01 << N))
#define USBHSD_UEP_RX_EN(N)           ((uint16_t) (0x01 << N))

#define USBHSD_UEP_RXDMA(N)           (*((volatile uint32_t*) (USBHSD_UEP_RXDMA_BASE + (N - 1) * 0x04)))
#define USBHSD_UEP_RXBUF(N)           ((uint8_t *) (*((volatile uint32_t*) (USBHSD_UEP_RXDMA_BASE + (N - 1) * 0x04))) + 0x20000000)
#define USBHSD_UEP_TXCTRL(N)          (*((volatile uint8_t*) (USBHSD_UEP_TXCTL_BASE + (N - 1) * 0x04)))
#define USBHSD_UEP_TXDMA(N)           (*((volatile uint32_t*) (USBHSD_UEP_TXDMA_BASE + (N - 1) * 0x04)))
#define USBHSD_UEP_TXBUF(N)           ((uint8_t*) (*((volatile uint32_t*) (USBHSD_UEP_TXDMA_BASE + (N - 1) * 0x04))) + 0x20000000)
#define USBHSD_UEP_TLEN(N)            (*((volatile uint16_t*) (USBHSD_UEP_TXLEN_BASE + (N - 1) * 0x04)))

/* USB SPEED TYPE */
#define USBHS_SPEED_TYPE_MASK         ((uint8_t) (0x03))
#define USBHS_SPEED_LOW               ((uint8_t) (0x02))
#define USBHS_SPEED_FULL              ((uint8_t) (0x00))
#define USBHS_SPEED_HIGH              ((uint8_t) (0x01))

/* usb device endpoint size define */
#define DEF_USBD_UEP0_SIZE           64     /* usb hs/fs device end-point 0 size */
/* HS */
#define DEF_USBD_HS_PACK_SIZE        512    /* usb hs device max bluk/int pack size */
#define DEF_USBD_HS_ISO_PACK_SIZE    1024   /* usb hs device max iso pack size */
/* FS */
#define DEF_USBD_FS_PACK_SIZE        64     /* usb fs device max bluk/int pack size */
#define DEF_USBD_FS_ISO_PACK_SIZE    1023   /* usb fs device max iso pack size */
/* LS */
#define DEf_USBD_LS_UEP0_SIZE        8      /* usb ls device end-point 0 size */
#define DEF_USBD_LS_PACK_SIZE        64     /* usb ls device max int pack size */

#define DEF_USBD_DESC_LEN(desc)         ((uint8_t) desc[0])
#define DEF_USBD_CFG_DESC_LEN(desc)     ((uint16_t)desc[2] + (uint16_t)(desc[3] << 8))

#define SET_REPORT_DEAL_OVER          0x00
#define SET_REPORT_WAIT_DEAL          0x01

// 设备描述符
static const uint8_t DevDescr[] = {
    0x12,                           // bLength
    0x01,                           // bDescriptorType
    0x00, 0x02,                     // bcdUSB
    0x00,                           // bDeviceClass
    0x00,                           // bDeviceSubClass
    0x00,                           // bDeviceProtocol
    DEF_USBD_UEP0_SIZE,             // bMaxPacketSize0
    0x3d, 0x41,                     // idVendor
    0x07, 0x21,                     // idProduct
    0x00, 0x00,                     // bcdDevice
    DEF_STRING_DESC_MANU,           // iManufacturer
    DEF_STRING_DESC_PROD,           // iProduct
    DEF_STRING_DESC_SERN,           // iSerialNumber
    0x01                            // bNumConfigurations
};

// 设备限定描述符
static const uint8_t QualDesc[] = {
    0x0A,                           // bLength
    0x06,                           // bDescriptorType
    0x00, 0x02,                     // bcdUSB
    0xFF,                           // bDeviceClass
    0xFF,                           // bDeviceSubClass
    0xFF,                           // bDeviceProtocol
    0x40,                           // bMaxPacketSize0
    0x01,                           // bNumConfigurations
    0x00                            // bReserved
};

/* Configuration Descriptor(HS) */
static const uint8_t CfgDescr_HS[] = {
    /* Configuration Descriptor */
    0x09,                           // bLength
    0x02,                           // bDescriptorType
    0x29, 0x00,                     // wTotalLength
    0x01,                           // bNumInterfaces
    0x01,                           // bConfigurationValue
    0x00,                           // iConfiguration (String Index)
    0x80,                           // bmAttributes
    0x23,                           // bMaxPower 70mA

    /* Interface Descriptor */
    0x09,                           // bLength
    0x04,                           // bDescriptorType (Interface)
    0x00,                           // bInterfaceNumber 0
    0x00,                           // bAlternateSetting
    0x02,                           // bNumEndpoints 2
    0x03,                           // bInterfaceClass
    0x00,                           // bInterfaceSubClass
    0x00,                           // bInterfaceProtocol
    0x00,                           // iInterface (String Index)

    /* HID Descriptor */
    0x09,                           // bLength
    0x21,                           // bDescriptorType
    0x11, 0x01,                     // bcdHID
    0x00,                           // bCountryCode
    0x01,                           // bNumDescriptors
    0x22,                           // bDescriptorType
#if !ARCADE_NATIVE_GAMEPAD
#if ARCADE_USB_WITH_REPORT_ID  
    0x25, 0x00,                     // wDescriptorLength
#else // ARCADE_USB_WITH_REPORT_ID
    0x21, 0x00,                     // wDescriptorLength
#endif // ARCADE_USB_WITH_REPORT_ID
#else // ARCADE_NATIVE_GAMEPAD
    0x1a, 0x00,                     // wDescriptorLength
#endif // ARCADE_NATIVE_GAMEPAD

    /* Endpoint Descriptor */
    0x07,                           // bLength
    0x05,                           // bDescriptorType
    (DEF_UEP1 | DEF_UEP_OUT),       // bEndpointAddress: OUT Endpoint 1
    0x03,                           // bmAttributes
    0x00, 0x02,                     // wMaxPacketSize
    0x01,                           // bInterval: 125us

    /* Endpoint Descriptor */
    0x07,                           // bLength
    0x05,                           // bDescriptorType
    (DEF_UEP1 | DEF_UEP_IN),        // bEndpointAddress: IN Endpoint 1
    0x03,                           // bmAttributes
    0x00, 0x02,                     // wMaxPacketSize
    0x01                            // bInterval: 125us
};

/* Configuration Descriptor(FS) */
static const uint8_t  CfgDescr_FS[] = {
    /* Configuration Descriptor */
    0x09,                           // bLength
    0x02,                           // bDescriptorType
    0x29, 0x00,                     // wTotalLength
    0x01,                           // bNumInterfaces
    0x01,                           // bConfigurationValue
    0x00,                           // iConfiguration (String Index)
    0x80,                           // bmAttributes Remote Wakeup
    0x23,                           // bMaxPower 70mA

    /* Interface Descriptor */
    0x09,                           // bLength
    0x04,                           // bDescriptorType (Interface)
    0x00,                           // bInterfaceNumber 0
    0x00,                           // bAlternateSetting
    0x02,                           // bNumEndpoints 2
    0x03,                           // bInterfaceClass
    0x00,                           // bInterfaceSubClass
    0x00,                           // bInterfaceProtocol
    0x00,                           // iInterface (String Index)

    /* HID Descriptor */
    0x09,                           // bLength
    0x21,                           // bDescriptorType
    0x11, 0x01,                     // bcdHID
    0x00,                           // bCountryCode
    0x01,                           // bNumDescriptors
    0x22,                           // bDescriptorType
#if !ARCADE_NATIVE_GAMEPAD
#if ARCADE_USB_WITH_REPORT_ID  
    0x25, 0x00,                     // wDescriptorLength
#else // ARCADE_USB_WITH_REPORT_ID
    0x21, 0x00,                     // wDescriptorLength
#endif // ARCADE_USB_WITH_REPORT_ID
#else // ARCADE_NATIVE_GAMEPAD
    0x1a, 0x00,                     // wDescriptorLength
#endif // ARCADE_NATIVE_GAMEPAD

    /* Endpoint Descriptor */
    0x07,                           // bLength
    0x05,                           // bDescriptorType
    (DEF_UEP1 | DEF_UEP_OUT),       // bEndpointAddress: OUT Endpoint 1
    0x03,                           // bmAttributes
    0x40, 0x00,                     // wMaxPacketSize
    0x01,                           // bInterval: 1ms

    /* Endpoint Descriptor */
    0x07,                           // bLength
    0x05,                           // bDescriptorType
    (DEF_UEP1 | DEF_UEP_IN),        // bEndpointAddress: IN Endpoint 1
    0x03,                           // bmAttributes
    0x40, 0x00,                     // wMaxPacketSize
    0x01                            // bInterval: 1ms
};

#if ARCADE_NATIVE_GAMEPAD

// Gamepad HID Report(size:26 0x1a)
static const uint8_t HIDReportDesc_FS[] = {
    0x05, 0x01, // Usage Page: Generic Desktop
    0x09, 0x05, // Usage: Gamepad
    0xa1, 0x01, // Collection: Application
        0xa1, 0x00, // Collection: Physics
            0x05, 0x09, // Usage Page: Button
            0x19, 0x01, // Usage Minimum: 1
            0x29, 0x08, // Usage Maximum: 8
            0x15, 0x00, // Logical Minimum: 0
            0x25, 0x01, // Logical Maximum: 1
            0x95, 0x08, // Report Count: 8
            0x75, 0x01, // Report Size: 1
            0x81, 0x02, // Input (Data,Var,Abs)
        0xc0, // End Collection
    0xc0 // End Collection
};

#else // ARCADE_NATIVE_GAMEPAD

// HID Report Descriptor(FS)
static const uint8_t HIDReportDesc_FS[] = {
    0x06, // Usage Page
        0x00, 0xff, // Vendor Defined

    0x09, 0x01, // Usage
    0xa1, 0x01, // Main Collection
#if ARCADE_USB_WITH_REPORT_ID  
        0x85, 0x01, // Report ID
#endif // ARCADE_USB_WITH_REPORT_ID
        0x09, 0x02, // Usage
        0x15, 0x00, // Logical Minimun
        0x25, 0x03, // Logical Maximun
        0x75, 0x02, // Report Size
        0x95, 0x04, // Report Count
        0x81, 0x02, // Input

#if ARCADE_USB_WITH_REPORT_ID  
        0x85, 0x02, // Report ID
#endif // ARCADE_USB_WITH_REPORT_ID
        0x09, 0x03, // Usage
        0x15, 0x00, // Logical  Minimun
        0x26, 0xff, 0x00, // Logical  Maximun
        0x75, 0x08, // Report Size
        0x95, 0x40, // Report Count
        0x91, 0x02, // Output
    0xC0 // End Collection
};

#endif // ARCADE_NATIVE_GAMEPAD

/* HID Report Descriptor(HS) */
static const uint8_t* HIDReportDesc_HS = HIDReportDesc_FS;

/* USB Full-Speed Mode, Other speed configuration Descriptor */
static uint8_t TAB_USB_FS_OSC_DESC[sizeof(CfgDescr_HS)] = {
    /* Other parts are copied through the program */
    0x09, 0x07,
};

/* USB High-Speed Mode, Other speed configuration Descriptor */
static uint8_t TAB_USB_HS_OSC_DESC[sizeof(CfgDescr_FS)] = {
    /* Other parts are copied through the program */
    0x09, 0x07,
};

// 语言描述符
static const uint8_t LangDescr[] = {0x09, 0x04};

// 厂家信息
static const uint8_t Str_Manu[] = {'l', 0, 'i', 0, '_', 0, 'y', 0, 'u', 0, 'a', 0, 'n', 0, '_', 0, 'f', 0, 'a', 0, 'n', 0, 'g', 0};

// 产品信息
static const uint8_t Str_Product[] = {'A', 0, 'r', 0, 'c', 0, 'a', 0, 'd', 0, 'e', 0, ' ', 0, 'G', 0, 'a', 0, 'm', 0, 'e', 0, 's', 0};

// 序列号
static const uint8_t Str_Serial[] = {'1', 0, '1', 0, '4', 0, '5', 0, '1', 0, '4', 0, '1', 0, '9', 0, '1', 0, '9', 0, '8', 0, '1', 0, '0', 0};

/******************************************************************************/
/* Variable Definition */

/* Global */
static const uint8_t* pUSBHS_Descr;

/* Setup Request */
static volatile uint8_t  USBHS_SetupReqCode;
static volatile uint8_t  USBHS_SetupReqType;
static volatile uint16_t USBHS_SetupReqValue;
static volatile uint16_t USBHS_SetupReqIndex;
static volatile uint16_t USBHS_SetupReqLen;

/* USB Device Status */
static volatile uint8_t  USBHS_DevConfig;
static volatile uint8_t  USBHS_DevAddr;
static volatile uint16_t USBHS_DevMaxPackLen;
static volatile uint8_t  USBHS_DevSpeed;
static volatile uint8_t  USBHS_DevSleepStatus;
static volatile uint8_t  USBHS_DevEnumStatus;

/* HID Class Command */
static volatile uint8_t USBHS_HidIdle;
static volatile uint8_t USBHS_HidProtocol;
static volatile uint16_t Hid_Report_Ptr;

static uint8_t HID_Report_Buffer[DEF_USBD_HS_PACK_SIZE];               // HID Report Buffer

/* Endpoint Buffer */
__attribute__ ((aligned(4))) static uint8_t USBHS_EP0_Buf[DEF_USBD_UEP0_SIZE];
__attribute__ ((aligned(4))) static uint8_t USBHS_EP1_Rx_Buf[DEF_USBD_HS_PACK_SIZE];
__attribute__ ((aligned(4))) static uint8_t USBHS_EP1_Tx_Buf[DEF_USBD_HS_PACK_SIZE];

/* Endpoint tx busy flag */
static volatile uint8_t USBHS_Endp_Busy[DEF_UEP_NUM];

static void usb_hs_endp_init(void) {
    uint8_t i = 0;

    R16_U2EP_TX_EN = RB_EP0_EN | RB_EP1_EN;
    R16_U2EP_RX_EN = RB_EP0_EN | RB_EP1_EN;

    R32_U2EP0_MAX_LEN  = DEF_USBD_UEP0_SIZE;
    R32_U2EP1_MAX_LEN  = DEF_USBD_HS_PACK_SIZE;

    R32_U2EP0_DMA    = (uint32_t) ((uint8_t*) USBHS_EP0_Buf);
    R32_U2EP1_RX_DMA = (uint32_t) ((uint8_t*) USBHS_EP1_Rx_Buf);
    R32_U2EP1_TX_DMA = (uint32_t) ((uint8_t*) USBHS_EP1_Tx_Buf);

    R8_U2EP0_TX_CTRL = USBHS_UEP_T_RES_NAK;
    R8_U2EP0_RX_CTRL = USBHS_UEP_R_RES_ACK;
    R8_U2EP1_RX_CTRL = USBHS_UEP_R_RES_ACK;
    R8_U2EP1_TX_CTRL = USBHS_UEP_T_RES_NAK;

    /* Clear End-points Busy Status */
    for (i = 0; i < DEF_UEP_NUM; i++) {
        USBHS_Endp_Busy[i] = 0;
    }
}

void arcade_usb_hs_init() {
    R8_USBHS_PLL_CTRL = USBHS_PLL_EN;
    R16_PIN_CONFIG |= RB_PIN_USB2_EN;

    R8_USB2_CTRL = USBHS_UD_RST_LINK | USBHS_UD_PHY_SUSPENDM;
    R8_USB2_INT_EN = USBHS_UDIE_BUS_RST | USBHS_UDIE_SUSPEND | USBHS_UDIE_BUS_SLEEP | USBHS_UDIE_LPM_ACT | USBHS_UDIE_TRANSFER | USBHS_UDIE_LINK_RDY;
    usb_hs_endp_init();
    R8_USB2_BASE_MODE = USBHS_UD_SPEED_HIGH;
    R8_USB2_CTRL = USBHS_UD_DEV_EN | USBHS_UD_DMA_EN | USBHS_UD_LPM_EN | USBHS_UD_PHY_SUSPENDM;
    PFIC_EnableIRQ(USB2_DEVICE_IRQn);
}

void arcade_usb_hs_deinit() {
    R8_USBHS_PLL_CTRL &= ~USBHS_PLL_EN;
    R16_PIN_CONFIG &= ~RB_PIN_USB2_EN;

    R8_USB2_CTRL |= USBHS_UD_RST_SIE;
    R8_USB2_CTRL &= ~USBHS_UD_RST_SIE;
    PFIC_DisableIRQ(USB2_DEVICE_IRQn);
}

int arcade_usb_hs_input(uint8_t* buf, size_t len) {
    if (arcade_usb_hs_input_busy()) return -EBUSY;

#if ARCADE_USB_WITH_REPORT_ID  
    if (len >= DEF_USBD_HS_PACK_SIZE) return -EINVAL;
#else // ARCADE_USB_WITH_REPORT_ID
    if (len > DEF_USBD_HS_PACK_SIZE) return -EINVAL;
#endif // ARCADE_USB_WITH_REPORT_ID

    USBHS_Endp_Busy[DEF_UEP1] |= DEF_UEP_BUSY;

#if ARCADE_USB_WITH_REPORT_ID  
    USBHSD_UEP_TXBUF(DEF_UEP1)[0] = 0x01; // Report ID
    memcpy(USBHSD_UEP_TXBUF(DEF_UEP1), buf + 1, len);
    USBHSD_UEP_TLEN(DEF_UEP1) = len + 1;
#else // ARCADE_USB_WITH_REPORT_ID
    memcpy(USBHSD_UEP_TXBUF(DEF_UEP1), buf, len);
    DevEP1_IN_Deal(len);
    USBHSD_UEP_TLEN(DEF_UEP1) = len;
#endif // ARCADE_USB_WITH_REPORT_ID
    USBHSD_UEP_TXCTRL(DEF_UEP1) &= ~USBHS_UEP_T_RES_MASK;
    USBHSD_UEP_TXCTRL(DEF_UEP1) |= USBHS_UEP_T_RES_ACK;

    return 0;
}

bool arcade_usb_hs_input_busy() {
    return USBHS_Endp_Busy[DEF_UEP1] & DEF_UEP_BUSY;
}

int arcade_usb_hs_endp_input(uint8_t endp, uint8_t* pbuf, size_t len, enum arcade_usb_hs_transfer_mode mode) {
    /* DMA config, endp_ctrl config, endp_len config */
    if ((endp >= DEF_UEP1) && (endp <= DEF_UEP15)) {
        if (R16_U2EP_TX_EN & USBHSD_UEP_TX_EN(endp)) {
            if ((USBHS_Endp_Busy[endp] & DEF_UEP_BUSY) == 0x00) {
                /* end-point buffer mode is single buffer */
                switch (mode) {
                    case ARCADE_USB_HS_TRANSFER_DMA:
                        USBHSD_UEP_TXDMA(endp) = (uint32_t) pbuf;
                        break;
                    case ARCADE_USB_HS_TRANSFER_CPY:
                        memcpy(USBHSD_UEP_TXBUF(endp), pbuf, len);
                        break;
                    default:
                        return -ENOTSUP;
                }

                /* Set end-point busy */
                USBHS_Endp_Busy[endp] |= DEF_UEP_BUSY;

                /* end-point n response tx ack */
                USBHSD_UEP_TLEN(endp) = len;
                USBHSD_UEP_TXCTRL(endp) &= ~USBHS_UEP_T_RES_MASK;
                USBHSD_UEP_TXCTRL(endp) |= USBHS_UEP_T_RES_ACK;
            } else {
                return -EBUSY;
            }
        } else {
            return -EIO;
        }
    } else {
        return -ENXIO;
    }

    return 0;
}

static inline int usb_device_get_string_scriptor_handler() {
    uint8_t len, size;

    switch ((uint8_t) (USBHS_SetupReqValue & 0xFF)) {
        /* Descriptor 0, Language descriptor */
        case DEF_STRING_DESC_LANG:
            pUSBHS_Descr = LangDescr;
            size = sizeof(LangDescr);
            break;

        /* Descriptor 1, Manufacturers String descriptor */
        case DEF_STRING_DESC_MANU:
            pUSBHS_Descr = Str_Manu;
            size = sizeof(Str_Manu);
            break;

        /* Descriptor 2, Product String descriptor */
        case DEF_STRING_DESC_PROD:
            pUSBHS_Descr = Str_Product;
            size = sizeof(Str_Product);
            break;

        /* Descriptor 3, Serial-number String descriptor */
        case DEF_STRING_DESC_SERN:
            pUSBHS_Descr = Str_Serial;
            size = sizeof(Str_Serial);
            break;

        default:
            return -ENOENT;
    }

    len = size + 2;

    if (USBHS_SetupReqLen > len) USBHS_SetupReqLen = len;      //实际需上传总长度

    len = (USBHS_SetupReqLen >= DEF_USBD_UEP0_SIZE) ? DEF_USBD_UEP0_SIZE : USBHS_SetupReqLen;   //最大长度为64字节

    if (len < 2) return -ENOMEM;

    USBHS_EP0_Buf[0] = len;
    USBHS_EP0_Buf[1] = 0x03;
    memcpy(USBHS_EP0_Buf + 2, pUSBHS_Descr, size);  //拷贝函数
    pUSBHS_Descr += size;

    return 0;
}

static inline int usb_device_get_scriptor_handler() {
    uint16_t len = 0U;

    switch ((uint8_t) (USBHS_SetupReqValue >> 8)) {
        /* get usb device descriptor */
        case USB_DESCR_TYP_DEVICE:
            pUSBHS_Descr = DevDescr;
            len = DEF_USBD_DESC_LEN(DevDescr);
            break;

        /* get usb configuration descriptor */
        case USB_DESCR_TYP_CONFIG:
            /* Query current usb speed */
            if (R8_USB2_MIS_ST & USBHS_UDMS_HS_MOD) {
                /* High speed mode */
                USBHS_DevSpeed = USBHS_SPEED_HIGH;
                USBHS_DevMaxPackLen = DEF_USBD_HS_PACK_SIZE;
            } else {
                /* Full speed mode */
                USBHS_DevSpeed = USBHS_SPEED_FULL;
                USBHS_DevMaxPackLen = DEF_USBD_FS_PACK_SIZE;
            }

            /* Load usb configuration descriptor by speed */
            if (USBHS_DevSpeed == USBHS_SPEED_HIGH) {
                /* High speed mode */
                pUSBHS_Descr = CfgDescr_HS;
                len = DEF_USBD_CFG_DESC_LEN(CfgDescr_HS);
            } else {
                /* Full speed mode */
                pUSBHS_Descr = CfgDescr_FS;
                len = DEF_USBD_CFG_DESC_LEN(CfgDescr_FS);
            }
            break;

        /* get usb string descriptor */
        case USB_DESCR_TYP_STRING:
            return usb_device_get_string_scriptor_handler();

        /* get usb device qualify descriptor */
        case USB_DESCR_TYP_QUALIF:
            pUSBHS_Descr = QualDesc;
            len = DEF_USBD_DESC_LEN(QualDesc);
            break;

        /* get usb BOS descriptor */
        case USB_DESCR_TYP_BOS:
            /* USB 2.00 DO NOT support BOS descriptor */
            return -ENOTSUP;

        /* get usb other-speed descriptor */
        case USB_DESCR_TYP_SPEED:
            if (USBHS_DevSpeed == USBHS_SPEED_HIGH) {
                /* High speed mode */
                memcpy(&TAB_USB_HS_OSC_DESC[2], &CfgDescr_FS[2], DEF_USBD_CFG_DESC_LEN(CfgDescr_FS) - 2);
                pUSBHS_Descr = (uint8_t*) &TAB_USB_HS_OSC_DESC[0];
                len = DEF_USBD_CFG_DESC_LEN(CfgDescr_FS);
            } else if (USBHS_DevSpeed == USBHS_SPEED_FULL) {
                /* Full speed mode */
                memcpy(&TAB_USB_FS_OSC_DESC[2], &CfgDescr_HS[2], DEF_USBD_CFG_DESC_LEN(CfgDescr_HS) - 2);
                pUSBHS_Descr = (uint8_t*) &TAB_USB_FS_OSC_DESC[0];
                len = DEF_USBD_CFG_DESC_LEN(CfgDescr_HS);
            } else {
                return -ENOTSUP;
            }
            break;

        case USB_DESCR_TYP_REPORT:
            if (USBHS_DevSpeed == USBHS_SPEED_HIGH) {
                pUSBHS_Descr = HIDReportDesc_HS;
                len = sizeof(HIDReportDesc_FS); // sizeof(HIDReportDesc_HS);
            } else {
                pUSBHS_Descr = HIDReportDesc_FS;
                len = sizeof(HIDReportDesc_FS);
            }
            break;

        case USB_DESCR_TYP_HID:
            if (USBHS_SetupReqIndex == 0x00) {
                if (USBHS_DevSpeed == USBHS_SPEED_HIGH) {
                    pUSBHS_Descr = &CfgDescr_HS[18];
                    len = 9;
                } else {
                    pUSBHS_Descr = &CfgDescr_FS[18];
                    len = 9;
                }
            } else {
                return -ENOTSUP;
            }
            break;

        default :
            return -ENOTSUP;
    }

    /* Copy Descriptors to Endp0 DMA buffer */
    if (USBHS_SetupReqLen > len) USBHS_SetupReqLen = len;
    len = (USBHS_SetupReqLen >= DEF_USBD_UEP0_SIZE) ? DEF_USBD_UEP0_SIZE : USBHS_SetupReqLen;
    memcpy(USBHS_EP0_Buf, pUSBHS_Descr, len);
    pUSBHS_Descr += len;

    return 0;
}

static inline int usb_device_clear_feature_handler() {
    if ((USBHS_SetupReqType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_DEVICE) {
        /* clear one device feature */
        if ((uint8_t) (USBHS_SetupReqValue & 0xFF) == 0x01) {
            /* clear usb sleep status, device not prepare to sleep */
            USBHS_DevSleepStatus &= ~0x01;
        } else {
            return -ENOTSUP;
        }
    } else if ((USBHS_SetupReqType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_ENDP) {
        /* Set End-point Feature */
        if ((uint8_t) (USBHS_SetupReqValue & 0xFF) == USB_REQ_FEAT_ENDP_HALT) {
            /* Clear End-point Feature */
            switch((uint8_t) (USBHS_SetupReqIndex & 0xFF)) {
                case (DEF_UEP1 | DEF_UEP_OUT):
                    /* Set End-point 1 OUT ACK */
                    R8_U2EP1_RX_CTRL = USBHS_UEP_R_RES_ACK;
                    break;

                case (DEF_UEP1 | DEF_UEP_IN):
                    /* Set End-point 1 IN NAK */
                    R8_U2EP1_TX_CTRL = USBHS_UEP_T_RES_NAK;
                    break;

                default:
                    return -ENXIO;
            }
        } else {
            return -ENOTSUP;
        }
    } else {
        return -ENOTSUP;
    }

    return 0;
}

static inline int usb_device_set_feature_handler() {
    if ((USBHS_SetupReqType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_DEVICE) {
        /* Set Device Feature */
        if ((uint8_t) (USBHS_SetupReqValue & 0xFF) == USB_REQ_FEAT_REMOTE_WAKEUP) {
            if (((USBHS_DevSpeed == USBHS_SPEED_HIGH) && (CfgDescr_HS[7] & 0x20)) ||
                ((USBHS_DevSpeed == USBHS_SPEED_FULL) && (CfgDescr_FS[7] & 0x20))) {
                /* Set Wake-up flag, device prepare to sleep */
                USBHS_DevSleepStatus |= 0x01;
            } else {
                return -ENOTSUP;
            }
        } else if ((uint8_t) (USBHS_SetupReqValue & 0xFF) == 0x02) {

        } else {
            return -ENOTSUP;
        }
    } else if((USBHS_SetupReqType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_ENDP) {
        /* Set End-point Feature */
        if ((uint8_t) (USBHS_SetupReqValue & 0xFF) == USB_REQ_FEAT_ENDP_HALT) {
            /* Set end-points status stall */
            switch((uint8_t) (USBHS_SetupReqIndex & 0xFF)) {
                case (DEF_UEP1 | DEF_UEP_OUT):
                    /* Set End-point 1 OUT STALL */
                    R8_U2EP1_RX_CTRL = (R8_U2EP1_RX_CTRL & ~USBHS_UEP_R_RES_MASK) | USBHS_UEP_R_RES_STALL;
                    break;

                case (DEF_UEP1 | DEF_UEP_IN):
                    /* Set End-point 1 IN STALL */
                    R8_U2EP1_TX_CTRL = (R8_U2EP1_TX_CTRL & ~USBHS_UEP_T_RES_MASK) | USBHS_UEP_T_RES_STALL;
                    break;

                default:
                    return -ENXIO;
            }
        }
    }

    return 0;
}

static inline int usb_device_get_status_handler() {
    USBHS_EP0_Buf[0] = 0x00;
    USBHS_EP0_Buf[1] = 0x00;

    if ((USBHS_SetupReqType & USB_REQ_RECIP_MASK ) == USB_REQ_RECIP_ENDP) {
        switch((uint8_t) (USBHS_SetupReqIndex & 0xFF)) {
            case (DEF_UEP1 | DEF_UEP_OUT):
                if (((R8_U2EP1_RX_CTRL) & USBHS_UEP_R_RES_MASK) == USBHS_UEP_R_RES_STALL) {
                    USBHS_EP0_Buf[0] = 0x01;
                }
                break;

            case (DEF_UEP1 | DEF_UEP_IN):
                if (((R8_U2EP1_TX_CTRL) & USBHS_UEP_T_RES_MASK) == USBHS_UEP_T_RES_STALL) {
                    USBHS_EP0_Buf[0] = 0x01;
                }
                break;

            default:
                return -ENXIO;
        }
    } else if ((USBHS_SetupReqType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_DEVICE) {
        if (USBHS_DevSleepStatus & 0x01) {
            USBHS_EP0_Buf[0] = 0x02;
        }
    }

    if (USBHS_SetupReqLen > 2) USBHS_SetupReqLen = 2;

    return 0;
}

static inline int usb_device_hid_handler(uint16_t* len) {
    switch (USBHS_SetupReqCode) {
        case HID_SET_REPORT:                            /* 0x09: SET_REPORT */
            Hid_Report_Ptr = 0;
            break;

        case HID_GET_REPORT:                            /* 0x01: GET_REPORT */
            if (USBHS_SetupReqIndex == 0x00) {
                Hid_Report_Ptr = 0;
                *len = (USBHS_SetupReqLen >= DEF_USBD_UEP0_SIZE) ? DEF_USBD_UEP0_SIZE : USBHS_SetupReqLen;
                memcpy(USBHS_EP0_Buf, &HID_Report_Buffer[Hid_Report_Ptr], *len);
                Hid_Report_Ptr += *len;
            } else {
                return -EIO;
            }
            break;

        case HID_SET_IDLE:                              /* 0x0A: SET_IDLE */
            if (USBHS_SetupReqIndex == 0x00) {
                USBHS_HidIdle = (uint8_t) (USBHS_SetupReqValue >> 8);
            } else {
                return -EIO;
            }
            break;

        case HID_SET_PROTOCOL:                          /* 0x0B: SET_PROTOCOL */
            if (USBHS_SetupReqIndex == 0x00) {
                USBHS_HidProtocol = (uint8_t) USBHS_SetupReqValue;
            } else {
                return -EIO;
            }
            break;

        case HID_GET_IDLE:                              /* 0x02: GET_IDLE */
            if (USBHS_SetupReqIndex == 0x00) {
                USBHS_EP0_Buf[0] = USBHS_HidIdle;
                *len = 1U;
            } else {
                return -EIO;
            }
            break;

        case HID_GET_PROTOCOL:                          /* 0x03: GET_PROTOCOL */
            if (USBHS_SetupReqIndex == 0x00) {
                USBHS_EP0_Buf[0] = USBHS_HidProtocol;
                *len = 1U;
            } else {
                return -EIO;
            }
            break;

        default:
            return -ENOTSUP;
    }

    return 0;
}

static inline void usb_device_setup_handler(uint8_t endp_num) {
    int ret;
    uint16_t len;

    switch (endp_num) {
        case DEF_UEP0:
            if (R8_U2EP0_RX_CTRL & USBHS_UEP_R_SETUP_IS) {
                /* Store All Setup Values */
                USBHS_SetupReqType  = pUSBHS_SetupReqPak->bRequestType;
                USBHS_SetupReqCode  = pUSBHS_SetupReqPak->bRequest;
                USBHS_SetupReqLen   = pUSBHS_SetupReqPak->wLength;
                USBHS_SetupReqValue = pUSBHS_SetupReqPak->wValue;
                USBHS_SetupReqIndex = pUSBHS_SetupReqPak->wIndex;

                len = 0;
                ret = 0;
                if ((USBHS_SetupReqType & USB_REQ_TYP_MASK) != USB_REQ_TYP_STANDARD) {
                    /* usb non-standard request processing */
                    if ((USBHS_SetupReqType & USB_REQ_TYP_MASK) == USB_REQ_TYP_CLASS) {
                        ret = usb_device_hid_handler(&len);
                    }
                } else {
                    /* usb standard request processing */
                    switch (USBHS_SetupReqCode) {
                        /* get device/configuration/string/report/... descriptors */
                        case USB_GET_DESCRIPTOR:
                            ret = usb_device_get_scriptor_handler();
                            break;

                        /* Set usb address */
                        case USB_SET_ADDRESS:
                            USBHS_DevAddr = (uint16_t) (USBHS_SetupReqValue & 0xFF);
                            break;

                        /* Get usb configuration now set */
                        case USB_GET_CONFIGURATION:
                            USBHS_EP0_Buf[0] = USBHS_DevConfig;
                            if (USBHS_SetupReqLen > 1) USBHS_SetupReqLen = 1;
                            break;

                        /* Set usb configuration to use */
                        case USB_SET_CONFIGURATION:
                            USBHS_DevConfig = (uint8_t) (USBHS_SetupReqValue & 0xFF);
                            USBHS_DevEnumStatus = 0x01;
                            break;

                        /* Clear or disable one usb feature */
                        case USB_CLEAR_FEATURE:
                            ret = usb_device_clear_feature_handler();
                            break;

                        /* set or enable one usb feature */
                        case USB_SET_FEATURE:
                            ret = usb_device_set_feature_handler();
                            break;

                        /* This request allows the host to select another setting for the specified interface  */
                        case USB_GET_INTERFACE:
                            USBHS_EP0_Buf[0] = 0x00;
                            if (USBHS_SetupReqLen > 1) USBHS_SetupReqLen = 1;
                            break;

                        case USB_SET_INTERFACE:
                            break;

                        /* host get status of specified device/interface/end-points */
                        case USB_GET_STATUS:
                            ret = usb_device_get_status_handler();
                            break;

                        default:
                            ret = -ENOTSUP;
                            break;
                    }
                }

                if (ret != 0) {
                    /* if one request not support, return stall */
                    R8_U2EP0_TX_CTRL = USBHS_UEP_T_TOG_DATA1 | USBHS_UEP_T_RES_STALL;
                    R8_U2EP0_RX_CTRL = USBHS_UEP_R_TOG_DATA1 | USBHS_UEP_R_RES_STALL;
                } else {
                    /* end-point 0 data Tx/Rx */
                    if (USBHS_SetupReqType & DEF_UEP_IN) {
                        /* tx */
                        len = (USBHS_SetupReqLen>DEF_USBD_UEP0_SIZE) ? DEF_USBD_UEP0_SIZE : USBHS_SetupReqLen;
                        USBHS_SetupReqLen -= len;
                        R16_U2EP0_T_LEN = len;
                        R8_U2EP0_TX_CTRL = USBHS_UEP_T_TOG_DATA1 | USBHS_UEP_T_RES_ACK;
                    } else {
                        /* rx */
                        if (USBHS_SetupReqLen == 0) {
                            R16_U2EP0_T_LEN = 0;
                            R8_U2EP0_TX_CTRL = USBHS_UEP_T_TOG_DATA1 | USBHS_UEP_T_RES_ACK;
                        } else {
                            R8_U2EP0_RX_CTRL = USBHS_UEP_R_TOG_DATA1 | USBHS_UEP_R_RES_ACK;
                        }
                    }
                }
            } else {
                /* end-point 0 data out interrupt */
                R8_U2EP0_RX_CTRL = USBHS_UEP_R_RES_NAK; // clear
                len = R16_U2EP0_RX_LEN;

                /* if any processing about rx, set it here */
                if ((USBHS_SetupReqType & USB_REQ_TYP_MASK) != USB_REQ_TYP_STANDARD) {
                    USBHS_SetupReqLen = 0;
                    /* Non-standard request end-point 0 Data download */
                    if (USBHS_SetupReqCode == CDC_SET_LINE_CODING) {

                    }
                } else {
                    /* Standard request end-point 0 Data download */
                }

                if (USBHS_SetupReqLen == 0) {
                    R16_U2EP0_T_LEN  = 0;
                    R8_U2EP0_TX_CTRL = USBHS_UEP_T_TOG_DATA1 | USBHS_UEP_T_RES_ACK;
                }
            }
            R8_U2EP0_RX_CTRL &= ~USBHS_UEP_R_DONE;
            break;

        /* end-point 1 data out interrupt */
        case  DEF_UEP1:
            if (R8_U2EP1_RX_CTRL & USBHS_UEP_R_TOG_MATCH) {
                /* Endp download */
                R8_U2EP1_RX_CTRL ^= USBHS_UEP_R_TOG_DATA1;
                R8_U2EP1_RX_CTRL = (R8_U2EP1_RX_CTRL & ~USBHS_UEP_R_RES_MASK) | USBHS_UEP_R_RES_NAK;
                // TODO:OUT返回的数据 长度为R16_U2EP1_RX_LEN
            }
            R8_U2EP1_RX_CTRL &= ~USBHS_UEP_R_DONE;
            break;

        default:
            return;
    }
}

static inline void usb_device_in_handler(uint8_t endp_num) {
    uint16_t len;

    switch (endp_num) {
        /* end-point 0 data in interrupt */
        case  DEF_UEP0:
            if (USBHS_SetupReqLen == 0) {
                R8_U2EP0_RX_CTRL = USBHS_UEP_R_TOG_DATA1 | USBHS_UEP_R_RES_ACK;
            }

            if ((USBHS_SetupReqType & USB_REQ_TYP_MASK) != USB_REQ_TYP_STANDARD) {
                /* Non-standard request endpoint 0 Data upload */
                if ((USBHS_SetupReqType & USB_REQ_TYP_MASK) == USB_REQ_TYP_CLASS) {
                    switch (USBHS_SetupReqCode) {
                        case HID_GET_REPORT:
                            len = USBHS_SetupReqLen >= DEF_USBD_UEP0_SIZE ? DEF_USBD_UEP0_SIZE : USBHS_SetupReqLen;
                            memcpy(USBHS_EP0_Buf, &HID_Report_Buffer[Hid_Report_Ptr], len);
                            USBHS_SetupReqLen -= len;
                            Hid_Report_Ptr += len;
                            R16_U2EP0_T_LEN = len;
                            R8_U2EP0_TX_CTRL ^= USBHS_UEP_T_TOG_DATA1;
                            R8_U2EP0_TX_CTRL = (R8_U2EP0_TX_CTRL & ~USBHS_UEP_T_RES_MASK) | USBHS_UEP_T_RES_ACK; // clear

                            break;
                        default:
                            break;
                    }
                }
            } else {
                /* Standard request endpoint 0 Data upload */
                switch (USBHS_SetupReqCode) {
                    case USB_GET_DESCRIPTOR:
                        len = USBHS_SetupReqLen >= DEF_USBD_UEP0_SIZE ? DEF_USBD_UEP0_SIZE : USBHS_SetupReqLen;
                        memcpy(USBHS_EP0_Buf, pUSBHS_Descr, len);
                        USBHS_SetupReqLen -= len;
                        pUSBHS_Descr += len;
                        R16_U2EP0_T_LEN = len;
                        R8_U2EP0_TX_CTRL ^= USBHS_UEP_T_TOG_DATA1;
                        R8_U2EP0_TX_CTRL = (R8_U2EP0_TX_CTRL & ~USBHS_UEP_T_RES_MASK) | USBHS_UEP_T_RES_ACK; // clear
                        break;

                    case USB_SET_ADDRESS:
                        R8_USB2_DEV_AD = USBHS_DevAddr;
                        break;

                    default:
                        R16_U2EP0_T_LEN = 0;
                        break;
                }
            }

            R8_U2EP0_TX_CTRL &= ~USBHS_UEP_T_DONE;
            break;

        /* end-point 1 data in interrupt */
        case DEF_UEP1:
            R16_U2EP1_T_LEN = 0;
            R8_U2EP1_TX_CTRL ^= USBHS_UEP_T_TOG_DATA1;
            R8_U2EP1_TX_CTRL = (R8_U2EP1_TX_CTRL & ~USBHS_UEP_T_RES_MASK) | USBHS_UEP_T_RES_NAK;
            USBHS_Endp_Busy[DEF_UEP1] &= ~DEF_UEP_BUSY;

            R8_U2EP1_TX_CTRL &= ~USBHS_UEP_T_DONE;
            break;

        default:
            break;
    }
}

static inline void usb_device_transfer_handler() {
    uint8_t intst;
    uint8_t endp_num;

    intst = R8_USB2_INT_ST;
    endp_num = intst & USBHS_UDIS_EP_ID_MASK;

    if (!(intst & USBHS_UDIS_EP_DIR)) {
        // SETUP/OUT Transaction
        usb_device_setup_handler(endp_num);
    } else {
        /* data-in stage processing */
        usb_device_in_handler(endp_num);
    }
}

static inline void usb_device_link_ready_handler() {
#ifdef  SUPPORT_USB_HSI
    USB_HSI->CAL_CR |= HSI_CAL_EN | HSI_CAL_VLD;
    USB_HSI->CAL_CR &= ~HSI_CAL_RST;
#endif

    R8_USB2_INT_FG = USBHS_UDIF_LINK_RDY;
}

static inline void usb_device_suspend_handler() {
    /* usb suspend interrupt processing */
    if (R8_USB2_MIS_ST & USBHS_UDMS_SUSPEND) {
        USBHS_DevSleepStatus |= 0x02;
        if (USBHS_DevSleepStatus == 0x03) {
            /* Handling usb sleep here */
        }
    } else {
        USBHS_DevSleepStatus &= ~0x02;
    }

    R8_USB2_INT_FG = USBHS_UDIF_SUSPEND;
}

static inline void usb_device_soft_reset() {
    /* usb reset interrupt processing */
    USBHS_DevConfig = 0;
    USBHS_DevAddr = 0;
    USBHS_DevSleepStatus = 0;
    USBHS_DevEnumStatus = 0;

    R8_USB2_DEV_AD = 0;
    usb_hs_endp_init();
    R8_USB2_INT_FG = USBHS_UDIF_BUS_RST;
}

/*********************************************************************
 * @fn      USBHS_IRQHandler
 *
 * @brief   This function handles USBHS exception.
 *
 * @return  none
 */
__INTERRUPT
__HIGH_CODE
void USB2_DEVICE_IRQHandler(void) {
    uint8_t intflag = R8_USB2_INT_FG;

    if (intflag & USBHS_UDIF_TRANSFER) {
        usb_device_transfer_handler();
    } else if (intflag & USBHS_UDIF_LINK_RDY) {
        usb_device_link_ready_handler();
    } else if( intflag & USBHS_UDIF_SUSPEND ) {
        usb_device_suspend_handler();
    } else if( intflag & USBHS_UDIF_BUS_RST ) {
        usb_device_soft_reset();
    } else {
        /* other interrupts */
        R8_USB2_INT_FG = intflag;
    }
}

#endif
