#ifndef USBD_USER_H
#define USBD_USER_H

#include <zephyr/usb/usbd.h>

int daplink_hid_init(struct usbd_context *usbd_ctx);

void main_blink_hid_led(void);
void main_blink_cdc_led(void);

#endif
