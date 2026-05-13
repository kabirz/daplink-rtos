#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include "DAP_config.h"
#include "DAP.h"

LOG_MODULE_REGISTER(usbd_dap_v2, LOG_LEVEL_INF);

#define DAP_V2_BUF_SIZE     DAP_PACKET_SIZE

struct dap_v2_desc {
	struct usb_association_descriptor iad;
	struct usb_if_descriptor if0;
	struct usb_ep_descriptor out_ep;
	struct usb_ep_descriptor in_ep;
	struct usb_ep_descriptor hs_out_ep;
	struct usb_ep_descriptor hs_in_ep;
	struct usb_desc_header nil;
};

struct dap_v2_data {
	struct dap_v2_desc *desc;
	const struct usb_desc_header **fs_desc;
	const struct usb_desc_header **hs_desc;
	atomic_t state;
};

#define DAP_V2_ENABLED      0
#define DAP_V2_OUT_BUSY     1
#define DAP_V2_IN_BUSY      2

static struct dap_v2_desc dap_v2_desc = {
	.iad = {
		.bLength = sizeof(struct usb_association_descriptor),
		.bDescriptorType = USB_DESC_INTERFACE_ASSOC,
		.bFirstInterface = 0,
		.bInterfaceCount = 1,
		.bFunctionClass = 0xFF,
		.bFunctionSubClass = 0,
		.bFunctionProtocol = 0,
		.iFunction = 0,
	},
	.if0 = {
		.bLength = sizeof(struct usb_if_descriptor),
		.bDescriptorType = USB_DESC_INTERFACE,
		.bInterfaceNumber = 0,
		.bAlternateSetting = 0,
		.bNumEndpoints = 2,
		.bInterfaceClass = 0xFF,
		.bInterfaceSubClass = 0,
		.bInterfaceProtocol = 0,
		.iInterface = 0,
	},
	.out_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_DESC_ENDPOINT,
		.bEndpointAddress = 0x02,
		.bmAttributes = USB_EP_TYPE_BULK,
		.wMaxPacketSize = sys_cpu_to_le16(64),
		.bInterval = 0,
	},
	.in_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_DESC_ENDPOINT,
		.bEndpointAddress = 0x82,
		.bmAttributes = USB_EP_TYPE_BULK,
		.wMaxPacketSize = sys_cpu_to_le16(64),
		.bInterval = 0,
	},
	.hs_out_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_DESC_ENDPOINT,
		.bEndpointAddress = 0x02,
		.bmAttributes = USB_EP_TYPE_BULK,
		.wMaxPacketSize = sys_cpu_to_le16(512),
		.bInterval = 0,
	},
	.hs_in_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_DESC_ENDPOINT,
		.bEndpointAddress = 0x82,
		.bmAttributes = USB_EP_TYPE_BULK,
		.wMaxPacketSize = sys_cpu_to_le16(512),
		.bInterval = 0,
	},
	.nil = { .bLength = 0, .bDescriptorType = 0 },
};

static const struct usb_desc_header *dap_v2_fs_desc[] = {
	(struct usb_desc_header *)&dap_v2_desc.iad,
	(struct usb_desc_header *)&dap_v2_desc.if0,
	(struct usb_desc_header *)&dap_v2_desc.out_ep,
	(struct usb_desc_header *)&dap_v2_desc.in_ep,
	(struct usb_desc_header *)&dap_v2_desc.nil,
	NULL,
};

static const struct usb_desc_header *dap_v2_hs_desc[] = {
	(struct usb_desc_header *)&dap_v2_desc.iad,
	(struct usb_desc_header *)&dap_v2_desc.if0,
	(struct usb_desc_header *)&dap_v2_desc.hs_out_ep,
	(struct usb_desc_header *)&dap_v2_desc.hs_in_ep,
	(struct usb_desc_header *)&dap_v2_desc.nil,
	NULL,
};

static uint8_t dap_v2_buf_out[DAP_V2_BUF_SIZE];
static uint8_t dap_v2_buf_in[DAP_V2_BUF_SIZE];

static void dap_v2_submit_out(struct usbd_class_data *c_data);
static void dap_v2_submit_in(struct usbd_class_data *c_data);

static void *dap_v2_get_desc(struct usbd_class_data *const c_data,
			     const enum usbd_speed speed)
{
	struct dap_v2_data *data = usbd_class_get_private(c_data);

	if (speed == USBD_SPEED_HS) {
		return (void *)data->hs_desc;
	}
	return (void *)data->fs_desc;
}

static int dap_v2_request(struct usbd_class_data *const c_data,
			  struct net_buf *const buf, const int err)
{
	struct udc_buf_info *bi = (struct udc_buf_info *)net_buf_user_data(buf);
	struct dap_v2_data *data = usbd_class_get_private(c_data);
	const uint8_t ep = bi->ep;
	const uint8_t out_ep = dap_v2_desc.out_ep.bEndpointAddress;
	const uint8_t in_ep = dap_v2_desc.in_ep.bEndpointAddress;

	if (err == -ECONNABORTED) {
		if (ep == out_ep) {
			atomic_clear_bit(&data->state, DAP_V2_OUT_BUSY);
		}
		if (ep == in_ep) {
			atomic_clear_bit(&data->state, DAP_V2_IN_BUSY);
		}
		net_buf_unref(buf);
		return 0;
	}

	if (ep == out_ep) {
		atomic_clear_bit(&data->state, DAP_V2_OUT_BUSY);
		if (err == 0 && buf->len > 0) {
			size_t len = MIN(buf->len, sizeof(dap_v2_buf_out));
			memcpy(dap_v2_buf_out, buf->data, len);

			uint32_t resp_len = DAP_ProcessCommand(dap_v2_buf_out,
							       dap_v2_buf_in);
			uint16_t rlen = (uint16_t)(resp_len & 0xFFFF);
			if (rlen > 0 && rlen <= sizeof(dap_v2_buf_in)) {
				struct net_buf *nbuf = usbd_ep_buf_alloc(c_data,
					in_ep, rlen);
				if (nbuf) {
					net_buf_add_mem(nbuf, dap_v2_buf_in, rlen);
					usbd_ep_enqueue(c_data, nbuf);
				}
			}
		}
		net_buf_unref(buf);
		dap_v2_submit_out(c_data);
		return 0;
	}

	if (ep == in_ep) {
		atomic_clear_bit(&data->state, DAP_V2_IN_BUSY);
		net_buf_unref(buf);
		dap_v2_submit_in(c_data);
		return 0;
	}

	net_buf_unref(buf);
	return 0;
}

static void dap_v2_submit_out(struct usbd_class_data *c_data)
{
	struct dap_v2_data *data = usbd_class_get_private(c_data);

	if (!atomic_test_bit(&data->state, DAP_V2_ENABLED)) return;
	if (atomic_test_and_set_bit(&data->state, DAP_V2_OUT_BUSY)) return;

	struct net_buf *buf = usbd_ep_buf_alloc(c_data,
		dap_v2_desc.out_ep.bEndpointAddress, DAP_V2_BUF_SIZE);
	if (buf == NULL) {
		atomic_clear_bit(&data->state, DAP_V2_OUT_BUSY);
		return;
	}

	if (usbd_ep_enqueue(c_data, buf)) {
		net_buf_unref(buf);
		atomic_clear_bit(&data->state, DAP_V2_OUT_BUSY);
	}
}

static void dap_v2_submit_in(struct usbd_class_data *c_data)
{
	struct dap_v2_data *data = usbd_class_get_private(c_data);

	if (!atomic_test_bit(&data->state, DAP_V2_ENABLED)) return;
	if (atomic_test_and_set_bit(&data->state, DAP_V2_IN_BUSY)) return;

	struct net_buf *buf = usbd_ep_buf_alloc(c_data,
		dap_v2_desc.in_ep.bEndpointAddress, DAP_V2_BUF_SIZE);
	if (buf == NULL) {
		atomic_clear_bit(&data->state, DAP_V2_IN_BUSY);
		return;
	}

	net_buf_add_mem(buf, dap_v2_buf_in, sizeof(dap_v2_buf_in));
	if (usbd_ep_enqueue(c_data, buf)) {
		net_buf_unref(buf);
		atomic_clear_bit(&data->state, DAP_V2_IN_BUSY);
	}
}

static int dap_v2_init(struct usbd_class_data *c_data)
{
	LOG_DBG("CMSIS-DAP v2 BULK class init");
	return 0;
}

static void dap_v2_enable(struct usbd_class_data *c_data)
{
	struct dap_v2_data *data = usbd_class_get_private(c_data);

	atomic_set_bit(&data->state, DAP_V2_ENABLED);
	dap_v2_submit_out(c_data);
	dap_v2_submit_in(c_data);
	LOG_INF("CMSIS-DAP v2 enabled");
}

static void dap_v2_disable(struct usbd_class_data *c_data)
{
	struct dap_v2_data *data = usbd_class_get_private(c_data);

	atomic_clear_bit(&data->state, DAP_V2_ENABLED);
	LOG_INF("CMSIS-DAP v2 disabled");
}

static struct usbd_class_api dap_v2_api = {
	.init    = dap_v2_init,
	.enable  = dap_v2_enable,
	.disable = dap_v2_disable,
	.request = dap_v2_request,
	.get_desc = dap_v2_get_desc,
};

static struct dap_v2_data dap_v2_data = {
	.desc    = &dap_v2_desc,
	.fs_desc = dap_v2_fs_desc,
	.hs_desc = dap_v2_hs_desc,
};

USBD_DEFINE_CLASS(dap_v2, &dap_v2_api, &dap_v2_data, NULL);
