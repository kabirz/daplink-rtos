#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include "DAP_config.h"

#define TARGET_UART_NODE DT_ALIAS(target_uart)

#if DT_NODE_EXISTS(TARGET_UART_NODE)
static const struct device *const target_uart = DEVICE_DT_GET(TARGET_UART_NODE);
#else
static const struct device *const target_uart = NULL;
#endif

static bool uart_initialized = false;
static uint32_t uart_baudrate = 115200;

typedef enum {
    DAP_UART_PARITY_NONE    = 0,
    DAP_UART_PARITY_ODD     = 1,
    DAP_UART_PARITY_EVEN    = 2,
    DAP_UART_PARITY_MARK    = 3,
    DAP_UART_PARITY_SPACE   = 4
} UART_Parity;

typedef enum {
    DAP_UART_STOP_BITS_1    = 0,
    DAP_UART_STOP_BITS_1_5  = 1,
    DAP_UART_STOP_BITS_2    = 2
} UART_StopBits;

typedef enum {
    DAP_UART_DATA_BITS_5    = 5,
    DAP_UART_DATA_BITS_6    = 6,
    DAP_UART_DATA_BITS_7    = 7,
    DAP_UART_DATA_BITS_8    = 8,
    DAP_UART_DATA_BITS_16   = 16
} UART_DataBits;

typedef enum {
    DAP_UART_FLOW_CONTROL_NONE     = 0,
    DAP_UART_FLOW_CONTROL_RTS_CTS  = 1,
    DAP_UART_FLOW_CONTROL_XON_XOFF = 2
} UART_FlowControl;

typedef struct {
    uint32_t           Baudrate;
    UART_DataBits      DataBits;
    UART_Parity        Parity;
    UART_StopBits      StopBits;
    UART_FlowControl   FlowControl;
} UART_Configuration;

int32_t uart_initialize(void)
{
    if (uart_initialized) return 1;
    if (!target_uart || !device_is_ready(target_uart)) return 0;
    uart_initialized = true;
    return 1;
}

int32_t uart_uninitialize(void)
{
    uart_initialized = false;
    return 1;
}

int32_t uart_reset(void)
{
    return 1;
}

int32_t uart_set_configuration(UART_Configuration *config)
{
    if (!uart_initialized) return 0;
    uart_baudrate = config->Baudrate;

    struct uart_config ucfg;
    ucfg.baudrate = config->Baudrate;
    ucfg.data_bits = (enum uart_config_data_bits)config->DataBits;
    ucfg.parity = (enum uart_config_parity)config->Parity;
    ucfg.stop_bits = (enum uart_config_stop_bits)config->StopBits;
    ucfg.flow_ctrl = (config->FlowControl == DAP_UART_FLOW_CONTROL_RTS_CTS) ?
                      UART_CFG_FLOW_CTRL_RTS_CTS : UART_CFG_FLOW_CTRL_NONE;
    return (uart_configure(target_uart, &ucfg) == 0) ? 1 : 0;
}

int32_t uart_get_configuration(UART_Configuration *config)
{
    config->Baudrate = uart_baudrate;
    config->DataBits = DAP_UART_DATA_BITS_8;
    config->Parity = DAP_UART_PARITY_NONE;
    config->StopBits = DAP_UART_STOP_BITS_1;
    config->FlowControl = DAP_UART_FLOW_CONTROL_NONE;
    return 1;
}

int32_t uart_write_free(void)
{
    if (!uart_initialized) return 0;
    return DAP_UART_TX_BUF_SIZE;
}

int32_t uart_write_data(uint8_t *data, uint16_t size)
{
    if (!uart_initialized) return 0;
    for (uint16_t i = 0; i < size; i++) {
        uart_poll_out(target_uart, data[i]);
    }
    return size;
}

int32_t uart_read_data(uint8_t *data, uint16_t size)
{
    if (!uart_initialized) return 0;
    uint16_t count = 0;
    while (count < size) {
        if (uart_poll_in(target_uart, &data[count]) == 0) {
            count++;
        } else {
            break;
        }
    }
    return count;
}

void uart_set_control_line_state(uint16_t ctrl_bmp)
{
    (void)ctrl_bmp;
}

void uart_software_flow_control(void)
{
}

void uart_enable_flow_control(bool enabled)
{
    (void)enabled;
}
