/*
 * usb_config.h
 *
 * CherryUSB device config for ch32_hello_world: a DFU "runtime" interface
 * (handled by a small custom class handler) plus a CDC ACM VCOM.
 */
#ifndef USB_CONFIG_H
#define USB_CONFIG_H

#define CONFIG_USB_PRINTF(...) ((void)0)
#define CONFIG_USB_DBG_LEVEL   0
#define CONFIG_USB_ALIGN_SIZE  4
#define USB_NOCACHE_RAM_SECTION

#define CONFIG_USB_DEVICE          1
#define CONFIG_USB_DEVICE_CDC_ACM  1

#define CONFIG_USBDEV_MAX_BUS             1
#define CONFIG_USBDEV_ADVANCE_DESC        1
#define CONFIG_USBDEV_REQUEST_BUFFER_LEN  512
#define CONFIG_USBDEV_EP_NUM              8

#define USBD_VID        0x1A86
#define USBD_PID        0xDF12
#define USBD_MAX_POWER  100

#endif /* USB_CONFIG_H */
