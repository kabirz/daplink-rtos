#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include "DAP_config.h"
#include "DAP.h"
#include "info.h"

#define DP_RDBUFF 0x0CU

DAP_Data_t DAP_Data;
volatile uint8_t DAP_TransferAbort;

static const char DAP_FW_Ver[] = DAP_FW_VER;

static uint8_t DAP_Info(uint8_t id, uint8_t *info)
{
    uint8_t length = 0U;

    switch (id) {
        case DAP_ID_VENDOR:
            length = DAP_GetVendorString((char *)info);
            break;
        case DAP_ID_PRODUCT:
            length = DAP_GetProductString((char *)info);
            break;
        case DAP_ID_SER_NUM:
            length = DAP_GetSerNumString((char *)info);
            break;
        case DAP_ID_DAP_FW_VER:
            length = (uint8_t)sizeof(DAP_FW_Ver);
            memcpy(info, DAP_FW_Ver, length);
            break;
        case DAP_ID_DEVICE_VENDOR:
            length = DAP_GetTargetDeviceVendorString((char *)info);
            break;
        case DAP_ID_DEVICE_NAME:
            length = DAP_GetTargetDeviceNameString((char *)info);
            break;
        case DAP_ID_BOARD_VENDOR:
            length = DAP_GetBoardVendorString((char *)info);
            break;
        case DAP_ID_BOARD_NAME:
            length = DAP_GetBoardNameString((char *)info);
            break;
        case DAP_ID_PRODUCT_FW_VER:
            length = DAP_GetProductFirmwareVersionString((char *)info);
            break;
        case DAP_ID_CAPABILITIES: {
            uint8_t caps = 0U;
            if (DAP_Data.debug_port != DAP_PORT_DISABLED) {
                caps |= 0x01;
            }
            if (DAP_SWD) {
                caps |= 0x02;
            }
            if (DAP_JTAG) {
                caps |= 0x04;
            }
            if (SWO_UART) {
                caps |= 0x08;
            }
            if (SWO_MANCHESTER) {
                caps |= 0x10;
            }
            caps |= 0x20;
            *info = caps;
            length = 1U;
            break;
        }
        case DAP_ID_TIMESTAMP_CLOCK:
            sys_put_le32(TIMESTAMP_CLOCK, info);
            info += 4;
            length = 4U;
            break;
        case DAP_ID_UART_RX_BUFFER_SIZE:
            sys_put_le16(DAP_UART_RX_BUF_SIZE, info);
            info += 2;
            length = 2U;
            break;
        case DAP_ID_UART_TX_BUFFER_SIZE:
            sys_put_le16(DAP_UART_TX_BUF_SIZE, info);
            info += 2;
            length = 2U;
            break;
        case DAP_ID_PACKET_COUNT:
            *info = DAP_PACKET_COUNT;
            length = 1U;
            break;
        case DAP_ID_PACKET_SIZE:
            sys_put_le16(DAP_PACKET_SIZE, info);
            info += 2;
            length = 2U;
            break;
        default:
            break;
    }
    return length;
}

static uint32_t DAP_HostStatus(const uint8_t *request, uint8_t *response)
{
    *response++ = DAP_OK;
    return ((2U << 16) | 1U);
}

static uint32_t DAP_Connect(const uint8_t *request, uint8_t *response)
{
    uint8_t port = *request;
    *response++ = DAP_OK;

    switch (port) {
        case DAP_PORT_AUTODETECT:
#if (DAP_SWD != 0)
            port = DAP_PORT_SWD;
#elif (DAP_JTAG != 0)
            port = DAP_PORT_JTAG;
#else
            port = DAP_PORT_DISABLED;
#endif
            break;
        case DAP_PORT_SWD:
#if (DAP_SWD == 0)
            port = DAP_PORT_DISABLED;
#endif
            break;
        case DAP_PORT_JTAG:
#if (DAP_JTAG == 0)
            port = DAP_PORT_DISABLED;
#endif
            break;
        default:
            port = DAP_PORT_DISABLED;
            break;
    }

    DAP_Data.debug_port = (uint8_t)port;
    DAP_Data.nominal_clock = DAP_DEFAULT_SWJ_CLOCK;
    *response++ = port;

    return ((2U << 16) | 2U);
}

static uint32_t DAP_Disconnect(uint8_t *response)
{
    DAP_Data.debug_port = DAP_PORT_DISABLED;
    DAP_Data.nominal_clock = 0U;
    *response++ = DAP_OK;
    return ((1U << 16) | 1U);
}

static uint32_t DAP_TransferConfigure(const uint8_t *request, uint8_t *response)
{
    DAP_Data.transfer.idle_cycles = *request++;
    DAP_Data.transfer.retry_count = *request++;
    DAP_Data.transfer.match_retry = sys_get_le16(request);
    request += 2;
    DAP_Data.transfer.match_mask = sys_get_le32(request);
    request += 4;
    *response++ = DAP_OK;
    return ((4U << 16) | 1U);
}

#if (DAP_SWD != 0)
static uint32_t DAP_Transfer(const uint8_t *request, uint8_t *response)
{
    uint8_t  request_value;
    uint32_t data;
    uint32_t retry;
    uint8_t  response_value;
    uint8_t  match_value;
    uint8_t  match_mask;
    uint8_t  post_read;

    *response++ = *request++;
    *response++ = 0U;
    response_value = 0U;

    request_value = *request++;
    data = 0U;

    if (request_value & DAP_TRANSFER_MATCH_VALUE) {
        match_value = *request++;
        match_mask  = *request++;
    }

    post_read = 0U;
    retry = DAP_Data.transfer.retry_count;

    do {
        response_value = DAP_TRANSFER_OK;

        if (request_value & DAP_TRANSFER_RnW) {
            if (request_value & DAP_TRANSFER_MATCH_MASK) {
                data = (uint32_t)*request++;
                if ((request_value & DAP_TRANSFER_MATCH_VALUE) == 0) {
                    match_value = *request++;
                    match_mask  = *request++;
                }
                do {
                    if (DAP_TransferAbort) {
                        response_value = DAP_TRANSFER_ERROR;
                        break;
                    }
                    response_value = SWD_TransferCheck(request_value, &data);
                } while ((response_value == DAP_TRANSFER_MISMATCH) && retry-- && !DAP_TransferAbort);
                if (response_value == DAP_TRANSFER_MISMATCH) {
                    response_value = DAP_TRANSFER_ERROR;
                }
            } else {
                if (request_value & DAP_TRANSFER_APnDP) {
                    if (post_read == 0U) {
                        do {
                            response_value = SWD_Transfer(request_value, NULL);
                        } while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
                        if (response_value != DAP_TRANSFER_OK) break;
                        post_read = 1U;
                    }
                } else {
                    do {
                        response_value = SWD_Transfer(request_value, &data);
                    } while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
                    if (response_value != DAP_TRANSFER_OK) break;
                    sys_put_le32(data, response);
                    response += 4;
                }
            }
        } else {
            if (request_value & DAP_TRANSFER_MATCH_MASK) {
                data = (uint32_t)*request++;
                if ((request_value & DAP_TRANSFER_MATCH_VALUE) == 0) {
                    match_value = *request++;
                    match_mask  = *request++;
                }
                do {
                    if (DAP_TransferAbort) {
                        response_value = DAP_TRANSFER_ERROR;
                        break;
                    }
                    response_value = SWD_TransferCheck(request_value, &data);
                } while ((response_value == DAP_TRANSFER_MISMATCH) && retry-- && !DAP_TransferAbort);
                if (response_value == DAP_TRANSFER_MISMATCH) {
                    response_value = DAP_TRANSFER_ERROR;
                }
            } else {
                data = sys_get_le32(request);
                request += 4;
                do {
                    response_value = SWD_Transfer(request_value, &data);
                } while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
            }
        }
        if (response_value != DAP_TRANSFER_OK) break;

        request_value = *request++;
    } while ((request_value & DAP_TRANSFER_APnDP) == 0U);

    if (post_read && (response_value == DAP_TRANSFER_OK)) {
        do {
            response_value = SWD_Transfer(DP_RDBUFF | DAP_TRANSFER_RnW, &data);
        } while ((response_value == DAP_TRANSFER_WAIT) && DAP_Data.transfer.retry_count && !DAP_TransferAbort);
        if (response_value == DAP_TRANSFER_OK) {
            sys_put_le32(data, response);
            response += 4;
        }
    }

    if (response_value == DAP_TRANSFER_OK) {
        *response++ = DAP_OK;
    } else {
        *response++ = response_value;
    }

    uint32_t req_count = (uint32_t)(request - (response - 6));
    uint32_t resp_count = (uint32_t)(response - (response - 6));

    return ((req_count << 16) | resp_count);
}
#endif

static uint32_t DAP_TransferBlock(const uint8_t *request, uint8_t *response)
{
    *response++ = *request++;
    *response++ = 0U;
#if (DAP_SWD != 0)
    uint8_t  request_value;
    uint32_t data;
    uint32_t block_count;
    uint32_t retry;
    uint8_t  response_value;

    request_value = *request++;

    block_count = sys_get_le16(request);
    request += 2;

    sys_put_le16((uint16_t)block_count, response);
    response += 2;

    if (request_value & DAP_TRANSFER_RnW) {
        while (block_count--) {
            retry = DAP_Data.transfer.retry_count;
            do {
                response_value = SWD_Transfer(request_value, &data);
            } while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
            if (response_value != DAP_TRANSFER_OK) break;
            sys_put_le32(data, response);
            response += 4;
        }
    } else {
        while (block_count--) {
            data = sys_get_le32(request);
            request += 4;
            retry = DAP_Data.transfer.retry_count;
            do {
                response_value = SWD_Transfer(request_value, &data);
            } while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);
            if (response_value != DAP_TRANSFER_OK) break;
        }
    }

    uint32_t req_count = (uint32_t)(request - (response - 6));
    uint32_t resp_count = (uint32_t)(response - (response - 6));

    if (response_value == DAP_TRANSFER_OK) {
        *(response - 4) = (uint8_t)(block_count);
        *(response - 3) = (uint8_t)(block_count >> 8);
        *response++ = DAP_OK;
    } else {
        *response++ = response_value;
    }

    return ((req_count << 16) | resp_count);
#else
    (void)request;
    *response++ = DAP_ERROR;
    return ((3U << 16) | 3U);
#endif
}

static uint32_t DAP_WriteAbort(const uint8_t *request, uint8_t *response)
{
#if (DAP_SWD != 0)
    uint32_t data;
    uint32_t retry;
    uint8_t  response_value;

    data = sys_get_le32(request);
    request += 4;

    retry = DAP_Data.transfer.retry_count;
    do {
        response_value = SWD_Transfer(0x0F, &data);
    } while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);

    if (response_value == DAP_TRANSFER_OK) {
        *response++ = DAP_OK;
    } else {
        *response++ = response_value;
    }
    return ((4U << 16) | 1U);
#else
    (void)request;
    *response++ = DAP_ERROR;
    return ((4U << 16) | 1U);
#endif
}

static uint32_t DAP_Delay(const uint8_t *request, uint8_t *response)
{
    uint32_t delay;
    delay = sys_get_le32(request);
    request += 4;

#if defined(__GNUC__)
    volatile uint32_t d = delay;
    while (d--);
#else
    volatile uint32_t d = delay;
    while (d--);
#endif

    *response++ = DAP_OK;
    return ((4U << 16) | 1U);
}

static uint32_t DAP_ResetTarget(uint8_t *response)
{
    *response++ = DAP_OK;
    return ((1U << 16) | 1U);
}

#if ((DAP_SWD != 0) || (DAP_JTAG != 0))
static uint32_t DAP_SWJ_Pins(const uint8_t *request, uint8_t *response)
{
    uint32_t value;
    uint32_t select;
    uint32_t wait;
    uint32_t timestamp;

    value  = (uint32_t)(*request++);
    select = (uint32_t)(*request++);
    wait = sys_get_le32(request);
    request += 4;

    if ((select & (1U << DAP_SWJ_SWCLK_TCK)) != 0U) {
        if ((value & (1U << DAP_SWJ_SWCLK_TCK)) != 0U) {
            PIN_SWCLK_TCK_SET();
        } else {
            PIN_SWCLK_TCK_CLR();
        }
    }
    if ((select & (1U << DAP_SWJ_SWDIO_TMS)) != 0U) {
        if ((value & (1U << DAP_SWJ_SWDIO_TMS)) != 0U) {
            PIN_SWDIO_TMS_SET();
        } else {
            PIN_SWDIO_TMS_CLR();
        }
    }
    if ((select & (1U << DAP_SWJ_TDI)) != 0U) {
        PIN_TDI_OUT(value >> DAP_SWJ_TDI);
    }
    if ((select & (1U << DAP_SWJ_nTRST)) != 0U) {
        PIN_nTRST_OUT(value >> DAP_SWJ_nTRST);
    }
    if ((select & (1U << DAP_SWJ_nRESET)) != 0U) {
        PIN_nRESET_OUT(value >> DAP_SWJ_nRESET);
    }

    if (wait != 0U) {
        wait = 1U;
        timestamp = wait;
        do {
            if ((select & (1U << DAP_SWJ_SWCLK_TCK)) != 0U) {
                if ((value >> DAP_SWJ_SWCLK_TCK) ^ PIN_SWCLK_TCK_IN()) continue;
            }
            if ((select & (1U << DAP_SWJ_SWDIO_TMS)) != 0U) {
                if ((value >> DAP_SWJ_SWDIO_TMS) ^ PIN_SWDIO_TMS_IN()) continue;
            }
            if ((select & (1U << DAP_SWJ_TDI)) != 0U) {
                if ((value >> DAP_SWJ_TDI) ^ PIN_TDI_IN()) continue;
            }
            if ((select & (1U << DAP_SWJ_nTRST)) != 0U) {
                if ((value >> DAP_SWJ_nTRST) ^ PIN_nTRST_IN()) continue;
            }
            if ((select & (1U << DAP_SWJ_nRESET)) != 0U) {
                if ((value >> DAP_SWJ_nRESET) ^ PIN_nRESET_IN()) continue;
            }
            break;
        } while (--wait);
    }

    value = (PIN_SWCLK_TCK_IN() << DAP_SWJ_SWCLK_TCK) |
            (PIN_SWDIO_TMS_IN() << DAP_SWJ_SWDIO_TMS) |
            (PIN_TDI_IN()       << DAP_SWJ_TDI)       |
            (PIN_TDO_IN()       << DAP_SWJ_TDO)       |
            (PIN_nTRST_IN()     << DAP_SWJ_nTRST)     |
            (PIN_nRESET_IN()    << DAP_SWJ_nRESET);

    *response++ = (uint8_t)value;
    return ((6U << 16) | 1U);
}
#endif

static void Set_DAP_Clock_Delay(uint32_t clock)
{
    DAP_Data.nominal_clock = clock;
    if (clock == 0U) {
        DAP_Data.clock_delay = 0U;
        DAP_Data.fast_clock = 1U;
    } else {
        DAP_Data.clock_delay = (CPU_CLOCK + clock/2) / clock;
        DAP_Data.clock_delay = (DAP_Data.clock_delay + (IO_PORT_WRITE_CYCLES_CORRECTED-1)) / IO_PORT_WRITE_CYCLES_CORRECTED;
        DAP_Data.fast_clock = (DAP_Data.clock_delay == 0U) ? 1U : 0U;
    }
}

static uint32_t DAP_SWJ_Clock(const uint8_t *request, uint8_t *response)
{
    uint32_t clock;
    clock = sys_get_le32(request);
    request += 4;

    Set_DAP_Clock_Delay(clock);

    *response++ = DAP_OK;
    return ((4U << 16) | 1U);
}

static uint32_t DAP_SWJ_Sequence(const uint8_t *request, uint8_t *response)
{
#if ((DAP_SWD != 0) || (DAP_JTAG != 0))
    uint32_t count;
    count = *request++;
    if (count == 0U) count = 256U;
    SWJ_Sequence(count, request);
    *response++ = DAP_OK;
    return (((count + 7U)/8U + 1U) << 16 | 1U);
#else
    (void)request;
    *response++ = DAP_ERROR;
    return ((1U << 16) | 1U);
#endif
}

static uint32_t DAP_SWD_Configure(const uint8_t *request, uint8_t *response)
{
#if (DAP_SWD != 0)
    uint8_t config = *request;
    DAP_Data.swd_conf.turnaround = (config & 0x03) + 1;
    DAP_Data.swd_conf.data_phase = (config >> 4) & 0x01;
    *response++ = DAP_OK;
    return ((1U << 16) | 1U);
#else
    (void)request;
    *response++ = DAP_ERROR;
    return ((1U << 16) | 1U);
#endif
}

static uint32_t DAP_SWD_Sequence(const uint8_t *request, uint8_t *response)
{
#if (DAP_SWD != 0)
    uint32_t sequence_info;
    uint32_t sequence_count;
    uint32_t request_count;
    uint32_t response_count;
    uint32_t count;

    *response++ = DAP_OK;
    request_count  = 1U;
    response_count = 1U;

    sequence_count = *request++;
    while (sequence_count--) {
        sequence_info = *request++;
        count = sequence_info & 0xFFU;
        if (count == 0U) count = 64U;
        count = (count + 7U) / 8U;

        if ((sequence_info & 0x02U) != 0U) {
            PIN_SWDIO_OUT_DISABLE();
        } else {
            PIN_SWDIO_OUT_ENABLE();
        }
        SWD_Sequence(sequence_info, request, response);
        if (sequence_count == 0U) {
            PIN_SWDIO_OUT_ENABLE();
        }

        if ((sequence_info & 0x02U) != 0U) {
            request_count++;
            response += count;
            response_count += count;
        } else {
            request += count;
            request_count += count + 1U;
        }
    }

    return ((request_count << 16) | response_count);
#else
    (void)request;
    *response++ = DAP_ERROR;
    return ((1U << 16) | 1U);
#endif
}

static uint32_t DAP_JTAG_Sequence(const uint8_t *request, uint8_t *response)
{
#if (DAP_JTAG != 0)
    uint32_t sequence_info;
    uint32_t sequence_count;
    uint32_t request_count;
    uint32_t response_count;
    uint32_t count;

    *response++ = DAP_OK;
    request_count  = 1U;
    response_count = 1U;

    sequence_count = *request++;
    while (sequence_count--) {
        sequence_info = *request++;
        count = sequence_info & 0xFFU;
        if (count == 0U) count = 64U;
        count = (count + 7U) / 8U;
        JTAG_Sequence(sequence_info, request, response);
        request += count;
        request_count += count + 1U;
        if ((sequence_info & 0x04U) != 0U) {
            response += count;
            response_count += count;
        }
    }

    return ((request_count << 16) | response_count);
#else
    (void)request;
    *response++ = DAP_ERROR;
    return ((1U << 16) | 1U);
#endif
}

static uint32_t DAP_JTAG_Configure(const uint8_t *request, uint8_t *response)
{
#if (DAP_JTAG != 0)
    uint8_t config;
    config = *request;
    (void)config;
    *response++ = DAP_OK;
    return ((1U << 16) | 1U);
#else
    (void)request;
    *response++ = DAP_ERROR;
    return ((1U << 16) | 1U);
#endif
}

static uint32_t DAP_JTAG_IDCode(const uint8_t *request, uint8_t *response)
{
#if (DAP_JTAG != 0)
    (void)request;
    uint32_t data;
    uint32_t retry;
    uint8_t response_value;

    retry = DAP_Data.transfer.retry_count;
    do {
        response_value = JTAG_Transfer(0U, &data);
    } while ((response_value == DAP_TRANSFER_WAIT) && retry-- && !DAP_TransferAbort);

    if (response_value == DAP_TRANSFER_OK) {
        *response++ = DAP_OK;
        sys_put_le32(data, response);
        response += 4;
        return ((1U << 16) | 5U);
    }
    *response++ = response_value;
    return ((1U << 16) | 1U);
#else
    (void)request;
    *response++ = DAP_ERROR;
    return ((1U << 16) | 1U);
#endif
}

#if ((SWO_UART != 0) || (SWO_MANCHESTER != 0))
static uint32_t DAP_SWO_Transport(const uint8_t *request, uint8_t *response)
{
    uint8_t transport;
    transport = *request;
    DAP_Data.swo_transport = transport;
    *response++ = DAP_OK;
    return ((1U << 16) | 1U);
}

static uint32_t DAP_SWO_Mode(const uint8_t *request, uint8_t *response)
{
    uint8_t mode;
    uint8_t status;

    mode = *request;
    status = DAP_OK;

    switch (mode) {
        case 0U:
#if (SWO_UART != 0)
            SWO_UART_Uninit();
#endif
#if (SWO_MANCHESTER != 0)
            SWO_Manchester_Uninit();
#endif
            DAP_Data.swo_mode = 0U;
            break;
        case 1U:
#if (SWO_UART != 0)
            SWO_UART_Init(DAP_Data.swo_baudrate);
            DAP_Data.swo_mode = 1U;
#else
            status = DAP_ERROR;
#endif
            break;
        case 2U:
#if (SWO_MANCHESTER != 0)
            SWO_Manchester_Init(DAP_Data.swo_baudrate);
            DAP_Data.swo_mode = 2U;
#else
            status = DAP_ERROR;
#endif
            break;
        default:
            status = DAP_ERROR;
            break;
    }

    *response++ = status;
    return ((1U << 16) | 1U);
}

static uint32_t DAP_SWO_Baudrate(const uint8_t *request, uint8_t *response)
{
    uint32_t baudrate;
    baudrate = sys_get_le32(request);
    request += 4;

    DAP_Data.swo_baudrate = baudrate;

    *response++ = DAP_OK;
    sys_put_le32(baudrate, response);
    response += 4;
    return ((4U << 16) | 5U);
}

static uint32_t DAP_SWO_Control(const uint8_t *request, uint8_t *response)
{
    uint8_t control;
    control = *request;
    DAP_Data.swo_control = control;

    if (control & DAP_SWO_CTRL_ENABLE) {
        if (DAP_Data.swo_mode == 1U) {
#if (SWO_UART != 0)
            SWO_UART_Control(1U);
#endif
        }
        if (DAP_Data.swo_mode == 2U) {
#if (SWO_MANCHESTER != 0)
            SWO_Manchester_Control(1U);
#endif
        }
    } else {
        if (DAP_Data.swo_mode == 1U) {
#if (SWO_UART != 0)
            SWO_UART_Control(0U);
#endif
        }
        if (DAP_Data.swo_mode == 2U) {
#if (SWO_MANCHESTER != 0)
            SWO_Manchester_Control(0U);
#endif
        }
    }

    *response++ = DAP_OK;
    return ((1U << 16) | 1U);
}

static uint32_t DAP_SWO_Status(uint8_t *response)
{
    DAP_Data.swo_status = 0U;

#if (SWO_UART != 0)
    if (DAP_Data.swo_mode == 1U) {
        DAP_Data.swo_status = DAP_SWO_STREAM_READY;
    }
#endif
#if (SWO_MANCHESTER != 0)
    if (DAP_Data.swo_mode == 2U) {
        DAP_Data.swo_status = DAP_SWO_STREAM_READY;
    }
#endif

    sys_put_le32(DAP_Data.swo_status, response);
    response += 4;
    return ((0U << 16) | 4U);
}

static uint32_t DAP_SWO_Data(const uint8_t *request, uint8_t *response)
{
    uint32_t count;
    uint8_t  swo_data;

    count = sys_get_le16(request);
    request += 2;

    sys_put_le16((uint16_t)count, response);
    response += 2;

    while (count--) {
        swo_data = DAP_Data.swo_data;
        *response++ = swo_data;
    }

    return ((2U << 16) | (2U + ((response[-3] << 0) | (response[-2] << 8))));
}
#endif

static uint8_t daplink_uart_transport;

static uint32_t DAP_UART_TransportCmd(const uint8_t *request, uint8_t *response)
{
    daplink_uart_transport = *request;

    if (daplink_uart_transport) {
        DAP_UART_Init();
        DAP_UART_Connect();
    } else {
        DAP_UART_Disconnect();
        DAP_UART_Uninit();
    }

    *response++ = DAP_OK;
    return ((1U << 16) | 1U);
}

static uint32_t DAP_UART_ConfigureCmd(const uint8_t *request, uint8_t *response)
{
    uint8_t config_type;
    uint8_t result;

    config_type = *request++;

    switch (config_type) {
        case DAP_UART_CONFIG_BAUD_RATE: {
            uint32_t baudrate;
            baudrate = sys_get_le32(request);
            request += 4;
            result = DAP_UART_Configure(baudrate);
            break;
        }
        case DAP_UART_CONFIG_LINE_CODING: {
            uint32_t baudrate;
            baudrate = sys_get_le32(request);
            request += 4;
            (void)baudrate;
            result = 1;
            break;
        }
        case DAP_UART_CONFIG_LINE_STATE: {
            uint8_t state;
            state = *request++;
            (void)state;
            result = 1;
            break;
        }
        default:
            result = 0;
            break;
    }

    *response++ = result ? DAP_OK : DAP_ERROR;
    return ((2U << 16) | 1U);
}

static uint32_t DAP_UART_ControlCmd(const uint8_t *request, uint8_t *response)
{
    uint8_t control;
    control = *request;

    if (control & DAP_UART_CTRL_ENABLE) {
        DAP_UART_Init();
    } else {
        DAP_UART_Uninit();
    }

    *response++ = DAP_OK;
    return ((1U << 16) | 1U);
}

static uint32_t DAP_UART_StatusCmd(uint8_t *response)
{
    uint32_t status = DAP_UART_Status();

    sys_put_le32(status, response);
    response += 4;
    return ((0U << 16) | 4U);
}

uint32_t DAP_ProcessCommand(const uint8_t *request, uint8_t *response)
{
    uint32_t num;

    if ((*request >= ID_DAP_Vendor0) && (*request <= ID_DAP_Vendor31)) {
        return DAP_ProcessVendorCommand(request, response);
    }
    if ((*request >= ID_DAP_VendorExFirst) && (*request <= ID_DAP_VendorExLast)) {
        return DAP_ProcessVendorCommandEx(request, response);
    }

    *response++ = *request;

    switch (*request++) {
        case ID_DAP_Info:
            num = DAP_Info(*request, response + 1);
            *response = (uint8_t)num;
            return ((2U << 16) + 2U + num);

        case ID_DAP_HostStatus:
            num = DAP_HostStatus(request, response);
            break;
        case ID_DAP_Connect:
            num = DAP_Connect(request, response);
            break;
        case ID_DAP_Disconnect:
            num = DAP_Disconnect(response);
            break;
        case ID_DAP_Delay:
            num = DAP_Delay(request, response);
            break;
        case ID_DAP_ResetTarget:
            num = DAP_ResetTarget(response);
            break;
        case ID_DAP_SWJ_Pins:
            num = DAP_SWJ_Pins(request, response);
            break;
        case ID_DAP_SWJ_Clock:
            num = DAP_SWJ_Clock(request, response);
            break;
        case ID_DAP_SWJ_Sequence:
            num = DAP_SWJ_Sequence(request, response);
            break;
        case ID_DAP_SWD_Configure:
            num = DAP_SWD_Configure(request, response);
            break;
        case ID_DAP_SWD_Sequence:
            num = DAP_SWD_Sequence(request, response);
            break;
        case ID_DAP_JTAG_Sequence:
            num = DAP_JTAG_Sequence(request, response);
            break;
        case ID_DAP_JTAG_Configure:
            num = DAP_JTAG_Configure(request, response);
            break;
        case ID_DAP_JTAG_IDCODE:
            num = DAP_JTAG_IDCode(request, response);
            break;
        case ID_DAP_TransferConfigure:
            num = DAP_TransferConfigure(request, response);
            break;
        case ID_DAP_Transfer:
            num = DAP_Transfer(request, response);
            break;
        case ID_DAP_TransferBlock:
            num = DAP_TransferBlock(request, response);
            break;
        case ID_DAP_WriteABORT:
            num = DAP_WriteAbort(request, response);
            break;
#if ((SWO_UART != 0) || (SWO_MANCHESTER != 0))
        case ID_DAP_SWO_Transport:
            num = DAP_SWO_Transport(request, response);
            break;
        case ID_DAP_SWO_Mode:
            num = DAP_SWO_Mode(request, response);
            break;
        case ID_DAP_SWO_Baudrate:
            num = DAP_SWO_Baudrate(request, response);
            break;
        case ID_DAP_SWO_Control:
            num = DAP_SWO_Control(request, response);
            break;
        case ID_DAP_SWO_Status:
            num = DAP_SWO_Status(response);
            break;
        case ID_DAP_SWO_Data:
            num = DAP_SWO_Data(request, response);
            break;
#endif
        case ID_DAP_UART_Transport:
            num = DAP_UART_TransportCmd(request, response);
            break;
        case ID_DAP_UART_Configure:
            num = DAP_UART_ConfigureCmd(request, response);
            break;
        case ID_DAP_UART_Control:
            num = DAP_UART_ControlCmd(request, response);
            break;
        case ID_DAP_UART_Status:
            num = DAP_UART_StatusCmd(response);
            break;
        default:
            *response = 0xFF;
            num = 1U;
            break;
    }

    return ((1U << 16) | num);
}

uint32_t DAP_ExecuteCommand(const uint8_t *request, uint8_t *response)
{
    if (*request == ID_DAP_ExecuteCommands) {
        uint32_t count;
        uint32_t num;

        *response++ = *request++;
        num = *request++;
        *response++ = (uint8_t)num;
        count = num;

        while (count--) {
            uint32_t n = DAP_ProcessCommand(request, response);
            uint32_t r = (n >> 16) & 0xFFFF;
            uint32_t s = (n >> 0) & 0xFFFF;
            request += r;
            response += s;
        }

        return ((uint32_t)(request - (response - 2 - num)) << 16 |
                (uint32_t)(response - (response - 2 - num)));
    }
    return DAP_ProcessCommand(request, response);
}

void DAP_Setup(void)
{
    DAP_Data.debug_port = DAP_PORT_DISABLED;
    DAP_Data.fast_clock = 0U;
    DAP_Data.clock_delay = 0U;
    DAP_Data.nominal_clock = DAP_DEFAULT_SWJ_CLOCK;
    DAP_Data.timestamp = 0U;
    DAP_Data.transfer.idle_cycles = 0U;
    DAP_Data.transfer.retry_count = DAP_TRANSFER_RETRIES;
    DAP_Data.transfer.match_retry = 0U;
    DAP_Data.transfer.match_mask = 0U;
#if (DAP_SWD != 0)
    DAP_Data.swd_conf.turnaround = 1U;
    DAP_Data.swd_conf.data_phase = 0U;
#endif
#if (DAP_JTAG != 0)
    DAP_Data.jtag_dev.count = 0U;
    DAP_Data.jtag_dev.index = 0U;
#endif
    DAP_TransferAbort = 0U;
    Set_DAP_Clock_Delay(DAP_DEFAULT_SWJ_CLOCK);
}

uint32_t DAP_ProcessVendorCommand(const uint8_t *request, uint8_t *response)
{
    (void)request;
    *response++ = 0xFF;
    return ((1U << 16) | 1U);
}

uint32_t DAP_ProcessVendorCommandEx(const uint8_t *request, uint8_t *response)
{
    (void)request;
    *response++ = 0xFF;
    return ((1U << 16) | 1U);
}

#if (TIMESTAMP_CLOCK != 0U)
uint32_t timestamp_clk_freq = TIMESTAMP_CLOCK;

uint32_t TIMESTAMP_GET(void)
{
    return k_cycle_get_32();
}
#endif
