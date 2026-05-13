#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include "DAP_config.h"
#include "DAP.h"

#define TARGET_UART_NODE DT_NODELABEL(usart2)

#if DT_NODE_EXISTS(TARGET_UART_NODE)
static const struct device *const target_uart = DEVICE_DT_GET(TARGET_UART_NODE);
#else
static const struct device *const target_uart = NULL;
#endif

static bool uart_active;
static uint32_t uart_baud = 115200;

void DAP_UART_Init(void)
{
    if (!target_uart || !device_is_ready(target_uart)) return;
    uart_active = true;
}

void DAP_UART_Uninit(void) { uart_active = false; }

uint8_t DAP_UART_Connect(void)
{
    return target_uart && device_is_ready(target_uart) ? 1 : 0;
}

uint8_t DAP_UART_Disconnect(void) { uart_active = false; return 1; }

uint8_t DAP_UART_Transport(uint8_t transport) { (void)transport; return 1; }

uint8_t DAP_UART_Configure(uint32_t baudrate)
{
    if (!target_uart) return 0;
    uart_baud = baudrate;
    struct uart_config cfg;
    if (uart_config_get(target_uart, &cfg)) return 0;
    cfg.baudrate = baudrate;
    return uart_configure(target_uart, &cfg) == 0 ? 1 : 0;
}

uint8_t DAP_UART_Control(uint8_t control)
{
    if (!target_uart) return 0;
    if (control & 1) uart_active = true;
    if (control & 2) uart_active = false;
    return 1;
}

uint8_t DAP_UART_Status(void)
{
    uint8_t s = 0;
    if (uart_active) s |= 1;
    if (target_uart) {
        uint8_t c;
        if (uart_poll_in(target_uart, &c) == 0) s |= 2;
    }
    s |= 4;
    return s;
}

uint16_t DAP_UART_Read(uint8_t *buf)
{
    if (!uart_active || !target_uart) return 0;
    uint16_t cnt = 0;
    while (cnt < DAP_PACKET_SIZE - 1) {
        if (uart_poll_in(target_uart, &buf[cnt]) != 0) break;
        cnt++;
    }
    return cnt;
}

uint16_t DAP_UART_Write(const uint8_t *buf)
{
    if (!uart_active || !target_uart) return 0;
    uint16_t len = buf[0];
    for (uint16_t i = 0; i < len && i < DAP_PACKET_SIZE - 1; i++) {
        uart_poll_out(target_uart, buf[1 + i]);
    }
    return len;
}
