#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart/uart_bridge.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/class/usbd_hid.h>
#include <zephyr/logging/log.h>
#include <sample_usbd.h>
#include "DAP.h"
#include "IO_Config.h"
#include "usbd_user.h"

LOG_MODULE_REGISTER(daplink, LOG_LEVEL_INF);

static struct usbd_context *sample_usbd;

#define HID_LED_NODE DT_ALIAS(hid_led)
#define CDC_LED_NODE DT_ALIAS(cdc_led)

#define DEVICE_DT_GET_COMMA(node_id) DEVICE_DT_GET(node_id)

static const struct device *uart_bridges[] = {
	DT_FOREACH_STATUS_OKAY(zephyr_uart_bridge, DEVICE_DT_GET_COMMA)
};

#if DT_NODE_EXISTS(HID_LED_NODE)
static const struct gpio_dt_spec hid_led = GPIO_DT_SPEC_GET(HID_LED_NODE, gpios);
#endif
#if defined(CDC_LED_NODE) && DT_NODE_EXISTS(CDC_LED_NODE)
static const struct gpio_dt_spec cdc_led = GPIO_DT_SPEC_GET(CDC_LED_NODE, gpios);
#endif

static volatile bool hid_activity;
static volatile bool cdc_activity;
static int64_t activity_tick;

enum usb_state {
    USB_DISCONNECTED,
    USB_CONNECTED,
};
static enum usb_state usb_state = USB_DISCONNECTED;

static struct k_timer daplink_timer;
static struct k_sem daplink_sem;

void main_blink_hid_led(void)
{
    hid_activity = true;
    activity_tick = k_uptime_get();
}

#if defined(CDC_LED_NODE) && DT_NODE_EXISTS(CDC_LED_NODE)
__attribute__((weak)) void main_blink_cdc_led(void)
{
    cdc_activity = true;
    activity_tick = k_uptime_get();
}
#endif

static void led_write(const struct gpio_dt_spec *led, bool on)
{
    if (led->port) gpio_pin_set_dt(led, on);
}

static void handle_leds(void)
{
    int64_t now = k_uptime_get();
    if (hid_activity && (now - activity_tick) > 250) {
        hid_activity = false;
    }
#if defined(CDC_LED_NODE) && DT_NODE_EXISTS(CDC_LED_NODE)
    if (cdc_activity && (now - activity_tick) > 250) {
        cdc_activity = false;
    }
    led_write(&cdc_led, cdc_activity);
#endif
    led_write(&hid_led, hid_activity);
}

__attribute__((weak)) void flash_prog_periodic(void) {}

static void timer_cb(struct k_timer *timer)
{
    k_sem_give(&daplink_sem);
}

static void msg_cb(struct usbd_context *const ctx, const struct usbd_msg *msg)
{
    if (usbd_can_detect_vbus(ctx)) {
        if (msg->type == USBD_MSG_VBUS_READY) {
            usbd_enable(ctx);
            usb_state = USB_CONNECTED;
        }
        if (msg->type == USBD_MSG_VBUS_REMOVED) {
            usbd_disable(ctx);
            usb_state = USB_DISCONNECTED;
        }
    }
    if (msg->type == USBD_MSG_CDC_ACM_LINE_CODING ||
        msg->type == USBD_MSG_CDC_ACM_CONTROL_LINE_STATE) {
        for (size_t i = 0; i < ARRAY_SIZE(uart_bridges); i++) {
            uart_bridge_settings_update(msg->dev, uart_bridges[i]);
        }
    }
}

static int enable_usb_device(void)
{
    sample_usbd = sample_usbd_init_device(msg_cb);
    if (sample_usbd == NULL) return -ENODEV;

    if (!usbd_can_detect_vbus(sample_usbd)) {
        int err = usbd_enable(sample_usbd);
        if (err) return err;
        usb_state = USB_CONNECTED;
    }
    return 0;
}

int main(void)
{
    LOG_INF("DAPLink CMSIS-DAP on Zephyr");

    swj_gpio_init();

    DAP_Setup();
    DAP_Data.debug_port = DAP_PORT_SWD;

#if DT_NODE_EXISTS(HID_LED_NODE)
    if (hid_led.port && device_is_ready(hid_led.port)) {
        gpio_pin_configure_dt(&hid_led, GPIO_OUTPUT_INACTIVE);
    }
#endif
#if DT_NODE_EXISTS(CDC_LED_NODE)
    if (cdc_led.port && device_is_ready(cdc_led.port)) {
        gpio_pin_configure_dt(&cdc_led, GPIO_OUTPUT_INACTIVE);
    }
#endif

    k_sem_init(&daplink_sem, 0, 1);
    k_timer_init(&daplink_timer, timer_cb, NULL);
    k_timer_start(&daplink_timer, K_MSEC(30), K_MSEC(30));

    int ret = enable_usb_device();
    if (ret) {
        LOG_ERR("USB failed: %d", ret);
        return 0;
    }

    ret = daplink_hid_init(sample_usbd);
    if (ret) LOG_ERR("HID failed: %d", ret);

    LOG_INF("DAPLink ready");

    uint32_t disk_check_tick = 0;

    while (1) {
        k_sem_take(&daplink_sem, K_FOREVER);
        handle_leds();

        uint32_t now = k_uptime_get_32();
        if (now - disk_check_tick > 2000) {
            disk_check_tick = now;
            flash_prog_periodic();
        }
    }
}
