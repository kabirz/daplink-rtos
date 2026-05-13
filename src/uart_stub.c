#include "DAP.h"

/* DAP_UART_* stubs for CMSIS-DAP protocol layer.
 * Actual UART bridging is handled by Zephyr uart-bridge driver. */

void DAP_UART_Init(void) {}
void DAP_UART_Uninit(void) {}
uint8_t DAP_UART_Connect(void) { return 1; }
uint8_t DAP_UART_Disconnect(void) { return 1; }
uint8_t DAP_UART_Transport(uint8_t transport) { (void)transport; return 1; }
uint8_t DAP_UART_Configure(uint32_t baudrate) { (void)baudrate; return 1; }
uint8_t DAP_UART_Control(uint8_t control) { (void)control; return 1; }
uint8_t DAP_UART_Status(void) { return 1; }
uint16_t DAP_UART_Read(uint8_t *buf) { (void)buf; return 0; }
uint16_t DAP_UART_Write(const uint8_t *buf) { (void)buf; return 0; }
