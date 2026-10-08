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

/* bcdDevice is part of the Windows hardware ID (USB\VID_xxxx&PID_xxxx&REV_xxxx),
 * so bumping it makes Windows treat the device as new and redo the whole WCID
 * driver installation instead of reusing a (possibly stale) cached result.
 * 'dfu-util -l' prints it as ver=xxxx. */
#define USBD_BCD_DEVICE 0x0202

static const uint8_t device_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0xEF, 0x02, 0x01, USBD_VID, USBD_PID, USBD_BCD_DEVICE, 0x01)
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

/* ========================================================================
 * Microsoft OS 1.0 descriptors (WCID)
 *
 * Without these, Windows has no idea which driver to bind to the DFU runtime
 * interface and `dfu-util -e` cannot open it (a manual Zadig step would be
 * needed).  With the Compatible ID below Windows automagically installs
 * WinUSB for interface 0, exactly like the ch32_dfu_boot bootloader does.
 *
 * The CDC ACM interfaces (1-2) are deliberately NOT listed: Windows then
 * keeps its inbox usbser.sys for them, so the VCOM still enumerates as a
 * COM port.
 *
 * Windows queries:
 *   GET_DESCRIPTOR(String, index 0xEE)      -> msos_string
 *   vendor request bRequest=0x20 wIndex=4   -> msos_compat_id
 *   vendor request bRequest=0x20 wIndex=5   -> msos_ext_prop (wValue = 0)
 * ======================================================================== */
#define APP_WINUSB_VENDOR_CODE 0x20U

/* DeviceInterfaceGUID of the DFU runtime interface.  Deliberately different
 * from the bootloader's so host tools can tell the two apart.
 *
 * Windows caches this property per device instance, so the value was changed
 * once (from {3f8b2c47-...}) together with bcdDevice to make sure the next
 * enumeration re-reads it instead of reusing the stale cache. */
#define APP_DFU_INTERFACE_GUID "{6d2f9a83-4c17-4e05-9b62-7a81c4f30d18}"

/* Microsoft OS String Descriptor, index 0xEE ("MSFT100" + vendor code) */
static const uint8_t msos_string[] = {
    0x12, 0x03,
    'M', 0x00, 'S', 0x00, 'F', 0x00, 'T', 0x00,
    '1', 0x00, '0', 0x00, '0', 0x00,
    APP_WINUSB_VENDOR_CODE,
    0x00,
};

/* Compatible ID Feature Descriptor: interface 0 (DFU runtime) -> WINUSB */
static const uint8_t msos_compat_id[] = {
    0x28, 0x00, 0x00, 0x00, /* dwLength = 16 + 24 * 1 */
    0x00, 0x01,             /* bcdVersion 1.0 */
    0x04, 0x00,             /* wIndex 0x0004 */
    0x01,                   /* bCount */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* reserved[7] */
    0x00,                   /* bFirstInterfaceNumber = 0 (DFU runtime) */
    0x01,                   /* reserved1 */
    0x57, 0x49, 0x4E, 0x55, /* compatibleID "WINUSB\0\0" */
    0x53, 0x42, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, /* subCompatibleID */
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* reserved2[6] */
};

/*
 * Extended Properties Feature Descriptor (DeviceInterfaceGUIDs), assembled at
 * init time from the plain ASCII GUID above so the UTF-16LE conversion cannot
 * be mistyped.
 *
 * Windows takes the response length from the first four bytes of this buffer,
 * so the declared length must cover exactly the bytes that are written:
 *   10 header + (4 dwSize + 4 dwPropertyDataType + 2 wPropertyNameLength
 *   + 40 L"DeviceInterfaceGUIDs" + 4 dwPropertyDataLength
 *   + 80 REG_MULTI_SZ payload) = 144
 */
#define MSOS_PROP_NAME        "DeviceInterfaceGUIDs"
#define MSOS_PROP_NAME_BYTES  ((uint32_t)sizeof(MSOS_PROP_NAME) * 2U)
/* REG_MULTI_SZ: the GUID string, its own NUL, and the list's NUL. */
#define MSOS_PROP_DATA_BYTES  (((uint32_t)sizeof(APP_DFU_INTERFACE_GUID) - 1U + 2U) * 2U)
#define MSOS_PROP_SECTION_LEN (4U + 4U + 2U + MSOS_PROP_NAME_BYTES + 4U + MSOS_PROP_DATA_BYTES)
#define MSOS_EXT_PROP_LEN     (10U + MSOS_PROP_SECTION_LEN)

static uint8_t msos_ext_prop[MSOS_EXT_PROP_LEN];

/* Returned for wValue != 0: a valid but empty property set. */
static const uint8_t msos_ext_prop_empty[] = {
    0x0a, 0x00, 0x00, 0x00, /* dwLength = 10 */
    0x00, 0x01,             /* bcdVersion 1.0 */
    0x05, 0x00,             /* wIndex 0x0005 */
    0x00, 0x00,             /* bCount = 0 */
};

/* CherryUSB indexes this array with setup->wValue, so keep two entries. */
static const uint8_t *msos_ext_prop_list[2];

static const struct usb_msosv1_descriptor composite_msosv1 = {
    .string = msos_string,
    .vendor_code = APP_WINUSB_VENDOR_CODE,
    .compat_id = msos_compat_id,
    .comp_id_property = msos_ext_prop_list,
};

static void msos_ext_prop_build(void)
{
    static const char prop_name[] = "DeviceInterfaceGUIDs";
    static const char guid[] = APP_DFU_INTERFACE_GUID;
    const uint32_t name_bytes = (uint32_t)sizeof(prop_name) * 2U;      /* + NUL */
    const uint32_t data_bytes = ((uint32_t)sizeof(guid) + 1U) * 2U;    /* + 2 NUL */
    const uint32_t section_len = 4U + 4U + 2U + name_bytes + 4U + data_bytes;
    uint32_t p = 0U;
    uint32_t i;

    msos_ext_prop[p++] = (uint8_t)(10U + section_len);
    msos_ext_prop[p++] = (uint8_t)((10U + section_len) >> 8);
    msos_ext_prop[p++] = 0x00U;
    msos_ext_prop[p++] = 0x00U;
    msos_ext_prop[p++] = 0x00U; /* bcdVersion 1.0 */
    msos_ext_prop[p++] = 0x01U;
    msos_ext_prop[p++] = 0x05U; /* wIndex 0x0005 */
    msos_ext_prop[p++] = 0x00U;
    msos_ext_prop[p++] = 0x01U; /* bCount = 1 */
    msos_ext_prop[p++] = 0x00U;

    msos_ext_prop[p++] = (uint8_t)(section_len);
    msos_ext_prop[p++] = (uint8_t)(section_len >> 8);
    msos_ext_prop[p++] = 0x00U;
    msos_ext_prop[p++] = 0x00U;
    msos_ext_prop[p++] = 0x07U; /* dwPropertyDataType: REG_MULTI_SZ */
    msos_ext_prop[p++] = 0x00U;
    msos_ext_prop[p++] = 0x00U;
    msos_ext_prop[p++] = 0x00U;
    msos_ext_prop[p++] = (uint8_t)(name_bytes);
    msos_ext_prop[p++] = (uint8_t)(name_bytes >> 8);

    for (i = 0U; i < (uint32_t)sizeof(prop_name); i++) {
        msos_ext_prop[p++] = (uint8_t)prop_name[i];
        msos_ext_prop[p++] = 0x00U;
    }

    msos_ext_prop[p++] = (uint8_t)(data_bytes);
    msos_ext_prop[p++] = (uint8_t)(data_bytes >> 8);
    msos_ext_prop[p++] = 0x00U;
    msos_ext_prop[p++] = 0x00U;

    for (i = 0U; i < (uint32_t)sizeof(guid); i++) {
        msos_ext_prop[p++] = (uint8_t)guid[i];
        msos_ext_prop[p++] = 0x00U;
    }
    /* REG_MULTI_SZ terminator */
    msos_ext_prop[p++] = 0x00U;
    msos_ext_prop[p++] = 0x00U;

    msos_ext_prop_list[0] = msos_ext_prop;
    msos_ext_prop_list[1] = msos_ext_prop_empty;
}

const struct usb_descriptor composite_descriptor = {
    .device_descriptor_callback         = device_descriptor_cb,
    .config_descriptor_callback         = config_descriptor_cb,
    .device_quality_descriptor_callback = device_quality_descriptor_cb,
    .other_speed_descriptor_callback    = other_speed_descriptor_cb,
    .string_descriptor_callback         = string_descriptor_cb,
    /* MS OS 1.0 (WCID) so Windows installs WinUSB for the DFU runtime
     * interface without a manual Zadig step */
    .msosv1_descriptor                  = &composite_msosv1,
    .msosv2_descriptor                  = NULL,
    .bos_descriptor                     = NULL,
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
    /* Assemble the WCID extended properties (DeviceInterfaceGUIDs). */
    msos_ext_prop_build();

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
