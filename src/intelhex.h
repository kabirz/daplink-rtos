#ifndef INTELHEX_H
#define INTELHEX_H

#include <stdint.h>

typedef enum {
    HEX_PARSE_OK = 0,
    HEX_PARSE_EOF,
    HEX_PARSE_UNALIGNED,
    HEX_PARSE_LINE_OVERRUN,
    HEX_PARSE_CKSUM_FAIL,
    HEX_PARSE_UNINIT,
    HEX_PARSE_FAILURE
} hexfile_parse_status_t;

void reset_hex_parser(void);

hexfile_parse_status_t parse_hex_blob(const uint8_t *hex_blob,
    uint32_t hex_blob_size, uint32_t *hex_parse_cnt,
    uint8_t *bin_buf, uint32_t bin_buf_size,
    uint32_t *bin_buf_address, uint32_t *bin_buf_cnt);

#endif
