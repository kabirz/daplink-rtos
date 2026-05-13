/*
 * Copyright (c) 2013-2021 ARM Limited. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * CMSIS-DAP v2.1.0 Protocol Definitions
 * Adapted for Zephyr RTOS
 */

#ifndef __DAP_H__
#define __DAP_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* DAP Firmware Version */
#ifdef  DAP_FW_V1
#define DAP_FW_VER                      "1.3.0"
#else
#define DAP_FW_VER                      "2.1.0"
#endif

/* DAP Command IDs */
#define ID_DAP_Info                     0x00U
#define ID_DAP_HostStatus               0x01U
#define ID_DAP_Connect                  0x02U
#define ID_DAP_Disconnect               0x03U
#define ID_DAP_TransferConfigure        0x04U
#define ID_DAP_Transfer                 0x05U
#define ID_DAP_TransferBlock            0x06U
#define ID_DAP_TransferAbort            0x07U
#define ID_DAP_WriteABORT               0x08U
#define ID_DAP_Delay                    0x09U
#define ID_DAP_ResetTarget              0x0AU
#define ID_DAP_SWJ_Pins                 0x10U
#define ID_DAP_SWJ_Clock                0x11U
#define ID_DAP_SWJ_Sequence             0x12U
#define ID_DAP_SWD_Configure            0x13U
#define ID_DAP_JTAG_Sequence            0x14U
#define ID_DAP_JTAG_Configure           0x15U
#define ID_DAP_JTAG_IDCODE              0x16U
#define ID_DAP_SWO_Transport            0x17U
#define ID_DAP_SWO_Mode                 0x18U
#define ID_DAP_SWO_Baudrate             0x19U
#define ID_DAP_SWO_Control              0x1AU
#define ID_DAP_SWO_Status               0x1BU
#define ID_DAP_SWO_Data                 0x1CU
#define ID_DAP_SWD_Sequence             0x1DU
#define ID_DAP_SWO_ExtendedStatus       0x1EU
#define ID_DAP_UART_Transport           0x1FU
#define ID_DAP_UART_Configure           0x20U
#define ID_DAP_UART_Control             0x22U
#define ID_DAP_UART_Status              0x21U

#define ID_DAP_Vendor0                  0x80U
#define ID_DAP_Vendor1                  0x81U
#define ID_DAP_Vendor2                  0x82U
#define ID_DAP_Vendor3                  0x83U
#define ID_DAP_Vendor4                  0x84U
#define ID_DAP_Vendor5                  0x85U
#define ID_DAP_Vendor6                  0x86U
#define ID_DAP_Vendor7                  0x87U
#define ID_DAP_Vendor8                  0x88U
#define ID_DAP_Vendor9                  0x89U
#define ID_DAP_Vendor10                 0x8AU
#define ID_DAP_Vendor11                 0x8BU
#define ID_DAP_Vendor12                 0x8CU
#define ID_DAP_Vendor13                 0x8DU
#define ID_DAP_Vendor14                 0x8EU
#define ID_DAP_Vendor15                 0x8FU
#define ID_DAP_Vendor16                 0x90U
#define ID_DAP_Vendor17                 0x91U
#define ID_DAP_Vendor18                 0x92U
#define ID_DAP_Vendor19                 0x93U
#define ID_DAP_Vendor20                 0x94U
#define ID_DAP_Vendor21                 0x95U
#define ID_DAP_Vendor22                 0x96U
#define ID_DAP_Vendor23                 0x97U
#define ID_DAP_Vendor24                 0x98U
#define ID_DAP_Vendor25                 0x99U
#define ID_DAP_Vendor26                 0x9AU
#define ID_DAP_Vendor27                 0x9BU
#define ID_DAP_Vendor28                 0x9CU
#define ID_DAP_Vendor29                 0x9DU
#define ID_DAP_Vendor30                 0x9EU
#define ID_DAP_Vendor31                 0x9FU

#define ID_DAP_VendorExFirst            0xA0U
#define ID_DAP_VendorExLast             0xFEU
#define ID_DAP_ExecuteCommands          0x7FU
#define ID_DAP_Invalid                  0xFFU

/* DAP Status Code */
#define DAP_OK                          0U
#define DAP_ERROR                       0xFFU

/* DAP ID */
#define DAP_ID_VENDOR                   1U
#define DAP_ID_PRODUCT                  2U
#define DAP_ID_SER_NUM                  3U
#define DAP_ID_DAP_FW_VER               4U
#define DAP_ID_DEVICE_VENDOR            5U
#define DAP_ID_DEVICE_NAME              6U
#define DAP_ID_BOARD_VENDOR             7U
#define DAP_ID_BOARD_NAME               8U
#define DAP_ID_PRODUCT_FW_VER           9U
#define DAP_ID_CAPABILITIES             0xF0U
#define DAP_ID_TIMESTAMP_CLOCK          0xF1U
#define DAP_ID_UART_RX_BUFFER_SIZE      0xFBU
#define DAP_ID_UART_TX_BUFFER_SIZE      0xFCU
#define DAP_ID_SWO_BUFFER_SIZE          0xFDU
#define DAP_ID_PACKET_COUNT             0xFEU
#define DAP_ID_PACKET_SIZE              0xFFU

/* DAP Host Status */
#define DAP_DEBUGGER_CONNECTED          0U
#define DAP_TARGET_RUNNING              1U

/* DAP Port */
#define DAP_PORT_AUTODETECT             0U
#define DAP_PORT_DISABLED               0U
#define DAP_PORT_SWD                    1U
#define DAP_PORT_JTAG                   2U

/* DAP SWJ */
#define DAP_SWJ_SWCLK_TCK               0U
#define DAP_SWJ_SWDIO_TMS               1U
#define DAP_SWJ_TDI                     2U
#define DAP_SWJ_TDO                     3U
#define DAP_SWJ_nTRST                   5U
#define DAP_SWJ_nRESET                  7U

/* DAP Transfer Request */
#define DAP_TRANSFER_APnDP              (1U<<0)
#define DAP_TRANSFER_RnW                (1U<<1)
#define DAP_TRANSFER_A2                 (1U<<2)
#define DAP_TRANSFER_A3                 (1U<<3)
#define DAP_TRANSFER_MATCH_VALUE        (1U<<4)
#define DAP_TRANSFER_MATCH_MASK         (1U<<5)
#define DAP_TRANSFER_TIMESTAMP          (1U<<7)

/* DAP Transfer Request (additional) */
#define DAP_TRANSFER_INVALID            0U

/* DAP Transfer Response */
#define DAP_TRANSFER_OK                 (1U<<0)
#define DAP_TRANSFER_WAIT               (1U<<1)
#define DAP_TRANSFER_FAULT              (1U<<2)
#define DAP_TRANSFER_ERROR              (1U<<3)
#define DAP_TRANSFER_MISMATCH           (1U<<4)

/* DAP SWO Trace */
#define DAP_SWO_OFF                     0U
#define DAP_SWO_UART                    1U
#define DAP_SWO_MANCHESTER              2U

#define DAP_SWO_CAPTURE_ACTIVE          (1U<<0)
#define DAP_SWO_CAPTURE_PAUSED          (1U<<1)
#define DAP_SWO_STREAM_ERROR            (1U<<6)
#define DAP_SWO_BUFFER_OVERRUN          (1U<<7)

/* DAP UART */
#define DAP_UART_TRANSPORT_NONE         0U
#define DAP_UART_TRANSPORT_USB_COM_PORT 1U
#define DAP_UART_TRANSPORT_DAP_COMMAND  2U

#define DAP_UART_CONTROL_RX_ENABLE      (1U<<0)
#define DAP_UART_CONTROL_RX_DISABLE     (1U<<1)
#define DAP_UART_CONTROL_RX_BUF_FLUSH   (1U<<2)
#define DAP_UART_CONTROL_TX_ENABLE      (1U<<4)
#define DAP_UART_CONTROL_TX_DISABLE     (1U<<5)
#define DAP_UART_CONTROL_TX_BUF_FLUSH   (1U<<6)

#define DAP_UART_STATUS_RX_ENABLED      (1U<<0)
#define DAP_UART_STATUS_RX_DATA_LOST    (1U<<1)
#define DAP_UART_STATUS_FRAMING_ERROR   (1U<<2)
#define DAP_UART_STATUS_PARITY_ERROR    (1U<<3)
#define DAP_UART_STATUS_TX_ENABLED      (1U<<4)

#define DAP_UART_CFG_ERROR_DATA_BITS    (1U<<0)
#define DAP_UART_CFG_ERROR_PARITY       (1U<<1)
#define DAP_UART_CFG_ERROR_STOP_BITS    (1U<<2)

#define DAP_UART_CONFIG_BAUD_RATE       0U
#define DAP_UART_CONFIG_LINE_CODING     1U
#define DAP_UART_CONFIG_LINE_STATE      2U

#define DAP_UART_CTRL_ENABLE            1U
#define DAP_UART_CTRL_CONNECT           2U

#define DAP_UART_STREAM_READY           1U
#define DAP_UART_RX_BUF_EMPTY           2U
#define DAP_UART_TX_BUF_EMPTY           4U

/* Debug Port Register Addresses */
#define DP_IDCODE                       0x00U
#define DP_ABORT                        0x00U
#define DP_CTRL_STAT                    0x04U
#define DP_WCR                          0x04U
#define DP_SELECT                       0x08U
#define DP_RESEND                       0x08U
#define DP_RDBUFF                       0x0CU

/* APCSR (AP Control and Status) */
#define AP_CSR                          0x00U
#define AP_TAR                          0x04U
#define AP_DRW                          0x0CU
#define AP_BD0                          0x10U
#define AP_BD1                          0x14U
#define AP_BD2                          0x18U
#define AP_BD3                          0x1CU
#define AP_CFG                          0xF4U
#define AP_BASE                         0xF8U
#define AP_IDR                          0xFCU

/* JTAG IR Codes */
#define JTAG_ABORT                      0x08U
#define JTAG_DPACC                      0x0AU
#define JTAG_APACC                      0x0BU
#define JTAG_IDCODE                     0x0EU
#define JTAG_BYPASS                     0x0FU

/* JTAG Sequence Info */
#define JTAG_SEQUENCE_TCK               0x3FU
#define JTAG_SEQUENCE_TMS               0x40U
#define JTAG_SEQUENCE_TDO               0x80U

/* SWD Sequence Info */
#define SWD_SEQUENCE_CLK                0x3FU
#define SWD_SEQUENCE_DIN                0x80U

/* DAP Data structure */
typedef struct {
    uint8_t     debug_port;             /* Debug Port */
    uint8_t     fast_clock;             /* Fast Clock Flag */
    uint8_t     padding[2];
    uint32_t    clock_delay;            /* Clock Delay */
    uint32_t    nominal_clock;          /* Nominal clock freq in Hz */
    uint32_t    timestamp;              /* Last captured Timestamp */
    struct {
        uint8_t   idle_cycles;          /* Idle cycles after transfer */
        uint8_t   padding[3];
        uint16_t  retry_count;          /* Retries after WAIT */
        uint16_t  match_retry;          /* Retries on value mismatch */
        uint32_t  match_mask;
    } transfer;
#if (DAP_SWD != 0)
    struct {
        uint8_t   turnaround;           /* Turnaround period */
        uint8_t   data_phase;           /* Always generate Data Phase */
    } swd_conf;
#endif
#if (DAP_JTAG != 0)
    struct {
        uint8_t   count;                /* Number of devices */
        uint8_t   index;                /* Device index */
#if (DAP_JTAG_DEV_CNT != 0)
        uint8_t   ir_length[DAP_JTAG_DEV_CNT];
        uint16_t  ir_before[DAP_JTAG_DEV_CNT];
        uint16_t  ir_after[DAP_JTAG_DEV_CNT];
#endif
    } jtag_dev;
#endif
} DAP_Data_t;

/* Global DAP Data */
extern DAP_Data_t DAP_Data;
extern volatile uint8_t DAP_TransferAbort;

/* DAP Functions */
void     DAP_Setup(void);
uint32_t DAP_ProcessCommand(const uint8_t *request, uint8_t *response);
uint32_t DAP_ExecuteCommand(const uint8_t *request, uint8_t *response);

/* SWJ Functions */
void     SWJ_Sequence(uint32_t count, const uint8_t *data);

/* SWD Functions */
void     SWD_Sequence(uint32_t info, const uint8_t *swdo, uint8_t *swdi);
uint8_t  SWD_Transfer(uint32_t request, uint32_t *data);
uint8_t  SWD_TransferCheck(uint32_t request, uint32_t *data);

/* JTAG Functions */
void     JTAG_Sequence(uint32_t info, const uint8_t *tdi, uint8_t *tdo);
uint8_t  JTAG_Transfer(uint32_t request, uint32_t *data);
uint8_t  JTAG_WriteIR(uint32_t request, uint32_t *data);
uint8_t  JTAG_IRScan(uint8_t dev_index, uint8_t *ir_data, uint8_t ir_length);

/* SWO Functions */
#if (SWO_UART != 0)
void     SWO_UART_Init(uint32_t baudrate);
void     SWO_UART_Uninit(void);
uint32_t SWO_UART_GetBaudrate(void);
void     SWO_UART_Control(uint32_t control);
#endif
#if (SWO_MANCHESTER != 0)
void     SWO_Manchester_Init(uint32_t baudrate);
void     SWO_Manchester_Uninit(void);
uint32_t SWO_Manchester_GetBaudrate(void);
void     SWO_Manchester_Control(uint32_t control);
#endif

/* DAP UART Functions */
void     DAP_UART_Init(void);
void     DAP_UART_Uninit(void);
uint8_t  DAP_UART_Connect(void);
uint8_t  DAP_UART_Disconnect(void);
uint8_t  DAP_UART_Transport(uint8_t transport);
uint8_t  DAP_UART_Configure(uint32_t baudrate);
uint8_t  DAP_UART_Control(uint8_t control);
uint8_t  DAP_UART_Status(void);
uint16_t DAP_UART_Read(uint8_t *buf);
uint16_t DAP_UART_Write(const uint8_t *buf);

/* DAP Vendor Functions */
uint32_t DAP_ProcessVendorCommand(const uint8_t *request, uint8_t *response);
uint32_t DAP_ProcessVendorCommandEx(const uint8_t *request, uint8_t *response);

/* DAP String Functions */
uint8_t  DAP_GetVendorString(char *str);
uint8_t  DAP_GetProductString(char *str);
uint8_t  DAP_GetSerNumString(char *str);
uint8_t  DAP_GetTargetDeviceVendorString(char *str);
uint8_t  DAP_GetTargetDeviceNameString(char *str);
uint8_t  DAP_GetBoardVendorString(char *str);
uint8_t  DAP_GetBoardNameString(char *str);
uint8_t  DAP_GetProductFirmwareVersionString(char *str);

/* DAP Queue Functions */
void     DAP_queue_init(void);
int      DAP_queue_send_buf(uint8_t *buf, int len);

#ifdef __cplusplus
}
#endif

#endif /* __DAP_H__ */
