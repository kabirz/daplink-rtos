#ifndef INFO_H
#define INFO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void info_init(void);

uint8_t DAP_GetVendorString(char *str);
uint8_t DAP_GetProductString(char *str);
uint8_t DAP_GetSerNumString(char *str);
uint8_t DAP_GetTargetDeviceVendorString(char *str);
uint8_t DAP_GetTargetDeviceNameString(char *str);
uint8_t DAP_GetBoardVendorString(char *str);
uint8_t DAP_GetBoardNameString(char *str);
uint8_t DAP_GetProductFirmwareVersionString(char *str);

#ifdef __cplusplus
}
#endif

#endif /* INFO_H */
