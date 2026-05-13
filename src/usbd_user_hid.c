#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/class/usbd_hid.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include "DAP_config.h"
#include "DAP.h"
#include "DAP_queue.h"

LOG_MODULE_REGISTER(usbd_hid_daplink, LOG_LEVEL_INF);

static const uint8_t hid_report_desc[] = {
    0x06, 0x00, 0xFF,
    0x09, 0x01,
    0xA1, 0x01,
    0x15, 0x00,
    0x26, 0xFF, 0x00,
    0x75, 0x08,
    0x95, DAP_PACKET_SIZE,
    0x09, 0x01,
    0x81, 0x02,
    0x95, DAP_PACKET_SIZE,
    0x09, 0x01,
    0x91, 0x02,
    0xC0
};

static uint8_t response_buf[DAP_PACKET_SIZE];
static const struct device *hid_dev;

__attribute__((weak)) void main_blink_hid_led(void) {}

static void hid_iface_ready(const struct device *dev, const bool ready)
{
    if (ready) {
        uint8_t *sbuf;
        int slen;
        if (DAP_queue_get_buf(&sbuf, &slen)) {
            hid_device_submit_report(dev, (uint16_t)slen, sbuf);
        }
    }
}

static int hid_set_report(const struct device *dev,
                          const uint8_t type, const uint8_t id,
                          const uint16_t len, const uint8_t *const buf)
{
    if (type != HID_REPORT_TYPE_OUTPUT || len > DAP_PACKET_SIZE || len == 0) {
        return -ENOTSUP;
    }

    uint32_t resp_len = DAP_ProcessCommand(buf, response_buf);
    uint16_t rlen = (uint16_t)(resp_len & 0xFFFF);
    if (rlen > 0) {
        DAP_queue_send_buf(response_buf, rlen);
        uint8_t *sbuf;
        int slen;
        if (DAP_queue_get_buf(&sbuf, &slen)) {
            hid_device_submit_report(dev, (uint16_t)slen, sbuf);
        }
    }
    main_blink_hid_led();
    return 0;
}

static void hid_output_report(const struct device *dev, const uint16_t len,
                              const uint8_t *const buf)
{
    if (len > DAP_PACKET_SIZE || len == 0) return;

    uint32_t resp_len = DAP_ProcessCommand(buf, response_buf);
    uint16_t rlen = (uint16_t)(resp_len & 0xFFFF);
    if (rlen > 0) {
        DAP_queue_send_buf(response_buf, rlen);
        uint8_t *sbuf;
        int slen;
        if (DAP_queue_get_buf(&sbuf, &slen)) {
            hid_device_submit_report(dev, (uint16_t)slen, sbuf);
        }
    }
    main_blink_hid_led();
}

static const struct hid_device_ops hid_ops = {
    .iface_ready   = hid_iface_ready,
    .set_report    = hid_set_report,
    .output_report = hid_output_report,
};

int daplink_hid_init(struct usbd_context *usbd_ctx)
{
    hid_dev = DEVICE_DT_GET_ONE(zephyr_hid_device);
    if (!device_is_ready(hid_dev)) {
        LOG_ERR("HID device not ready");
        return -ENODEV;
    }
    DAP_queue_init();
    return hid_device_register(hid_dev, hid_report_desc,
                               sizeof(hid_report_desc), &hid_ops);
}
