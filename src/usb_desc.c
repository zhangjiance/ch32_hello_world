/*
 * usb_desc.c
 *
 * Composite USB device: DFU runtime (interface 0) + CDC ACM VCOM (1-2).
 *
 *  - interface 0 (DFU runtime) answers DFU_DETACH (dfu-util -e).  Instead of
 *    performing an in-place DFU we set the BKP hand-shake and reset, so the
 *    ch32_dfu_boot bootloader comes up in DFU mode.
 *  - CDC ACM is a virtual serial port with a simple loopback.
 *
 * Ported from hpm_hello_world/src/usb_desc.c onto the CH32V30x USBHS port.
 */
#include "usbd_core.h"
#include "usbd_cdc_acm.h"

#include "boot_log.h"
#include "boot_trigger_port.h"

#include "usb_config.h"

/* Endpoints */
#define CDC_IN_EP  0x81
#define CDC_OUT_EP 0x02
#define CDC_INT_EP 0x83

/* DFU runtime interface + functional descriptor */
#define DFU_IF_LEN (9 + 9)

#define USB_CONFIG_SIZE (9 + DFU_IF_LEN + CDC_ACM_DESCRIPTOR_LEN)

/* ---------- descriptors ---------- */

static const uint8_t device_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0xEF, 0x02, 0x01, USBD_VID, USBD_PID, 0x0201, 0x01)
};

static const uint8_t config_descriptor_hs[] = {
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x03, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    /* DFU runtime, interface 0 */
    0x09, 0x04, 0x00, 0x00, 0x00, 0xFE, 0x01, 0x01, 0x04,
    0x09, 0x21, 0x0B, 0xFF, 0x00, 0x00, 0x10, 0x1A, 0x01,
    /* CDC ACM, interfaces 1-2 */
    CDC_ACM_DESCRIPTOR_INIT(0x01, CDC_INT_EP, CDC_OUT_EP, CDC_IN_EP, USB_BULK_EP_MPS_HS, 0x05),
};

static const uint8_t config_descriptor_fs[] = {
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x03, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    0x09, 0x04, 0x00, 0x00, 0x00, 0xFE, 0x01, 0x01, 0x04,
    0x09, 0x21, 0x0B, 0xFF, 0x00, 0x00, 0x10, 0x1A, 0x01,
    CDC_ACM_DESCRIPTOR_INIT(0x01, CDC_INT_EP, CDC_OUT_EP, CDC_IN_EP, USB_BULK_EP_MPS_FS, 0x05),
};

static const uint8_t device_quality_descriptor[] = {
    0x0a, USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER,
    0x00, 0x02, 0x00, 0x00, 0x00, 0x40, 0x01, 0x00,
};

static const uint8_t other_speed_config_descriptor_hs[] = {
    USB_OTHER_SPEED_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x03, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    0x09, 0x04, 0x00, 0x00, 0x00, 0xFE, 0x01, 0x01, 0x04,
    0x09, 0x21, 0x0B, 0xFF, 0x00, 0x00, 0x10, 0x1A, 0x01,
    CDC_ACM_DESCRIPTOR_INIT(0x01, CDC_INT_EP, CDC_OUT_EP, CDC_IN_EP, USB_BULK_EP_MPS_FS, 0x05),
};

static const uint8_t other_speed_config_descriptor_fs[] = {
    USB_OTHER_SPEED_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x03, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    0x09, 0x04, 0x00, 0x00, 0x00, 0xFE, 0x01, 0x01, 0x04,
    0x09, 0x21, 0x0B, 0xFF, 0x00, 0x00, 0x10, 0x1A, 0x01,
    CDC_ACM_DESCRIPTOR_INIT(0x01, CDC_INT_EP, CDC_OUT_EP, CDC_IN_EP, USB_BULK_EP_MPS_HS, 0x05),
};

static const char *string_descriptors[] = {
    (const char[]){ 0x09, 0x04 }, /* Langid */
    "CH32V30x",                   /* Manufacturer */
    "CH32V30x Hello World",       /* Product */
    "CH32APP0001",                /* Serial Number */
    "DFU Runtime",                /* iInterface 4 */
    "CDC ACM",                    /* iInterface 5 */
};

static const uint8_t *device_descriptor_cb(uint8_t speed)
{
    (void)speed;
    return device_descriptor;
}

static const uint8_t *config_descriptor_cb(uint8_t speed)
{
    return (speed == USB_SPEED_HIGH) ? config_descriptor_hs : config_descriptor_fs;
}

static const uint8_t *device_quality_descriptor_cb(uint8_t speed)
{
    (void)speed;
    return device_quality_descriptor;
}

static const uint8_t *other_speed_descriptor_cb(uint8_t speed)
{
    return (speed == USB_SPEED_HIGH) ? other_speed_config_descriptor_hs
                                     : other_speed_config_descriptor_fs;
}

static const char *string_descriptor_cb(uint8_t speed, uint8_t index)
{
    (void)speed;
    if (index >= (sizeof(string_descriptors) / sizeof(char *))) {
        return NULL;
    }
    return string_descriptors[index];
}

const struct usb_descriptor composite_descriptor = {
    .device_descriptor_callback         = device_descriptor_cb,
    .config_descriptor_callback         = config_descriptor_cb,
    .device_quality_descriptor_callback = device_quality_descriptor_cb,
    .other_speed_descriptor_callback    = other_speed_descriptor_cb,
    .string_descriptor_callback         = string_descriptor_cb,
};

/* ---------- DFU runtime class handler ---------- */

enum {
    DFU_DETACH    = 0,
    DFU_DNLOAD    = 1,
    DFU_UPLOAD    = 2,
    DFU_GETSTATUS = 3,
    DFU_CLRSTATUS = 4,
    DFU_GETSTATE  = 5,
    DFU_ABORT     = 6,
};

static int dfu_handler(uint8_t busid, struct usb_setup_packet *setup,
                       uint8_t **data, uint32_t *len)
{
    (void)busid;
    switch (setup->bRequest) {
        case DFU_DETACH:
            /* dfu-util -e: hand over to the bootloader.  Never returns. */
            BOOT_PRINTF("[APP] DFU_DETACH -> reboot into bootloader\r\n");
            boot_trigger_reboot_to_boot();
            return 0;
        case DFU_GETSTATUS: {
            static uint8_t status[6] = { 0, 0, 0, 0, 0, 0 }; /* OK, appIDLE */
            *data = status;
            *len = sizeof(status);
            return 0;
        }
        case DFU_GETSTATE: {
            static uint8_t state = 0; /* appIDLE */
            *data = &state;
            *len = 1;
            return 0;
        }
        default:
            return 0;
    }
}

/* ---------- CDC ACM (loopback) ---------- */

void usbd_cdc_acm_bulk_out(uint8_t busid, uint8_t ep, uint32_t nbytes);
void usbd_cdc_acm_bulk_in(uint8_t busid, uint8_t ep, uint32_t nbytes);

USB_MEM_ALIGNX uint8_t read_buffer[2][512];

static volatile uint8_t read_buffer_index;
static volatile bool ep_tx_busy_flag;
static volatile bool dtr_enable;

static struct usbd_interface intf0;
static struct usbd_interface intf1;
static struct usbd_interface dfu_intf;

static struct usbd_endpoint cdc_out_ep = { .ep_addr = CDC_OUT_EP, .ep_cb = usbd_cdc_acm_bulk_out };
static struct usbd_endpoint cdc_in_ep  = { .ep_addr = CDC_IN_EP,  .ep_cb = usbd_cdc_acm_bulk_in };

static void cdc_acm_start_read(uint8_t busid)
{
    read_buffer_index = 0;
    usbd_ep_start_read(busid, CDC_OUT_EP, &read_buffer[0][0], usbd_get_ep_mps(busid, CDC_OUT_EP));
}

/* Echo the received packet back on the IN endpoint (loopback). */
void usbd_cdc_acm_bulk_out(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    uint8_t index = read_buffer_index;

    read_buffer_index = (index == 0) ? 1 : 0;
    usbd_ep_start_write(busid, CDC_IN_EP, &read_buffer[index][0], nbytes);
    usbd_ep_start_read(busid, ep, &read_buffer[read_buffer_index][0], usbd_get_ep_mps(busid, ep));
}

void usbd_cdc_acm_bulk_in(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    if (((nbytes % usbd_get_ep_mps(busid, ep)) == 0) && nbytes) {
        usbd_ep_start_write(busid, ep, NULL, 0);   /* ZLP */
    } else {
        ep_tx_busy_flag = false;
    }
}

void usbd_cdc_acm_set_dtr(uint8_t busid, uint8_t intf, bool dtr)
{
    (void)busid;
    (void)intf;
    dtr_enable = dtr;
}

/* ---------- init ---------- */

static void usbd_event_handler(uint8_t busid, uint8_t event)
{
    switch (event) {
        case USBD_EVENT_CONFIGURED:
            cdc_acm_start_read(busid);
            BOOT_PRINTF("[APP] USB configured (DFU runtime + CDC ACM)\r\n");
            break;
        default:
            break;
    }
}

void app_usb_init(uint8_t busid, uintptr_t reg_base)
{
    usbd_desc_register(busid, &composite_descriptor);

    /* DFU runtime first -> interface number 0 */
    dfu_intf.class_interface_handler = dfu_handler;
    usbd_add_interface(busid, &dfu_intf);

    /* CDC ACM -> interfaces 1 and 2 */
    usbd_add_interface(busid, usbd_cdc_acm_init_intf(busid, &intf0));
    usbd_add_interface(busid, usbd_cdc_acm_init_intf(busid, &intf1));
    usbd_add_endpoint(busid, &cdc_out_ep);
    usbd_add_endpoint(busid, &cdc_in_ep);

    usbd_initialize(busid, reg_base, usbd_event_handler);
}
