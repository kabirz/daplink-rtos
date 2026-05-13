#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/class/usb_cdc.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include "DAP_config.h"
#include "DAP.h"

LOG_MODULE_REGISTER(usbd_cdc_daplink, LOG_LEVEL_ERR);

/* CDC ACM UART bridge between USB host and target UART */
static const struct device *const cdc_dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);

/* Ring buffers for USB <-> UART data */
static uint8_t usb_to_uart_buf[DAP_UART_RX_BUF_SIZE];
static uint8_t uart_to_usb_buf[DAP_UART_TX_BUF_SIZE];
static struct ring_buf usb_to_uart_rb;
static struct ring_buf uart_to_usb_rb;

static bool cdc_ready;
static struct k_sem cdc_tx_sem;

/* Forward data from USB host to target UART */
static void cdc_acm_irq_handler(const struct device *dev, void *user_data)
{
    ARG_UNUSED(user_data);

    while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
        if (uart_irq_rx_ready(dev)) {
            uint8_t buf[64];
            int recv = uart_fifo_read(dev, buf, sizeof(buf));
            if (recv > 0) {
                ring_buf_put(&usb_to_uart_rb, buf, recv);
            }
        }

        if (uart_irq_tx_ready(dev)) {
            uint8_t buf[64];
            int sent = ring_buf_get(&uart_to_usb_rb, buf, sizeof(buf));
            if (sent > 0) {
                uart_fifo_fill(dev, buf, sent);
            } else {
                uart_irq_tx_disable(dev);
            }
        }
    }
}

int daplink_cdc_acm_init(struct usbd_context *usbd_ctx)
{
    if (!device_is_ready(cdc_dev)) {
        LOG_ERR("CDC ACM device not ready");
        return -ENODEV;
    }

    ring_buf_init(&usb_to_uart_rb, sizeof(usb_to_uart_buf), usb_to_uart_buf);
    ring_buf_init(&uart_to_usb_rb, sizeof(uart_to_usb_buf), uart_to_usb_buf);
    k_sem_init(&cdc_tx_sem, 0, 1);

    /* Configure and enable UART interrupt */
    uart_irq_callback_set(cdc_dev, cdc_acm_irq_handler);
    uart_irq_rx_enable(cdc_dev);

    cdc_ready = true;
    LOG_INF("CDC ACM interface initialized");
    return 0;
}

/* UART bridge functions called from DAP_UART_* commands in DAP.c */
void DAP_UART_Init(void)
{
}

void DAP_UART_Uninit(void)
{
}

uint8_t DAP_UART_Connect(void)
{
    return 1;
}

uint8_t DAP_UART_Disconnect(void)
{
    return 1;
}

uint8_t DAP_UART_Transport(uint8_t transport)
{
    return 1;
}

uint8_t DAP_UART_Configure(uint32_t baudrate)
{
    return 1;
}

uint8_t DAP_UART_Control(uint8_t control)
{
    return 1;
}

uint8_t DAP_UART_Status(void)
{
    uint32_t status = DAP_UART_STREAM_READY;

    if (ring_buf_is_empty(&uart_to_usb_rb)) {
        status |= DAP_UART_RX_BUF_EMPTY;
    }
    if (ring_buf_is_empty(&usb_to_uart_rb)) {
        status |= DAP_UART_TX_BUF_EMPTY;
    }

    return (uint8_t)status;
}

uint16_t DAP_UART_Read(uint8_t *buf)
{
    return (uint16_t)ring_buf_get(&uart_to_usb_rb, buf, DAP_PACKET_SIZE - 1);
}

uint16_t DAP_UART_Write(const uint8_t *buf)
{
    uint16_t len = (uint16_t)buf[0]; /* First byte is length */
    /* Queue data for USB host */
    int ret = ring_buf_put(&usb_to_uart_rb, buf + 1, len);
    return (uint16_t)(ret > 0 ? ret : 0);
}
