#ifndef SWD_HOST_H
#define SWD_HOST_H

#include <stdint.h>
#include <stdbool.h>
#include "flash_blob.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CONNECT_NORMAL,
    CONNECT_UNDER_RESET,
} SWD_CONNECT_TYPE;

typedef enum {
    TARGET_STATE_RESET_RUN,
    TARGET_STATE_RESET_PROGRAM,
    TARGET_STATE_NO_DEBUG,
    TARGET_STATE_DEBUG,
    TARGET_STATE_HALT,
    TARGET_STATE_RUN,
    TARGET_STATE_POST_FLASH_RESET,
} target_state_t;

uint8_t swd_init(void);
uint8_t swd_off(void);
uint8_t swd_init_debug(void);
uint8_t swd_clear_errors(void);
uint8_t swd_read_dp(uint8_t adr, uint32_t *val);
uint8_t swd_write_dp(uint8_t adr, uint32_t val);
uint8_t swd_read_ap(uint32_t adr, uint32_t *val);
uint8_t swd_write_ap(uint32_t adr, uint32_t val);
uint8_t swd_read_word(uint32_t addr, uint32_t *val);
uint8_t swd_write_word(uint32_t addr, uint32_t val);
uint8_t swd_read_byte(uint32_t addr, uint8_t *val);
uint8_t swd_write_byte(uint32_t addr, uint8_t val);
uint8_t swd_read_memory(uint32_t address, uint8_t *data, uint32_t size);
uint8_t swd_write_memory(uint32_t address, uint8_t *data, uint32_t size);
uint8_t swd_read_core_register(uint32_t n, uint32_t *val);
uint8_t swd_write_core_register(uint32_t n, uint32_t val);
uint8_t swd_set_target_state_hw(target_state_t state);
uint8_t swd_set_target_state_sw(target_state_t state);
uint8_t swd_transfer_retry(uint32_t req, uint32_t *data);
void swd_set_reset_connect(SWD_CONNECT_TYPE type);
uint8_t JTAG2SWD(void);
uint8_t swd_flash_syscall_exec(const program_syscall_t *sysCallParam,
    uint32_t entry, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4,
    flash_algo_return_t return_type);

#ifdef __cplusplus
}
#endif

#endif /* SWD_HOST_H */
