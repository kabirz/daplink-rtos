#include <string.h>
#include "intelhex.h"

typedef enum {
    DATA_RECORD = 0,
    EOF_RECORD = 1,
    EXT_SEG_ADDR_RECORD = 2,
    START_SEG_ADDR_RECORD = 3,
    EXT_LINEAR_ADDR_RECORD = 4,
    START_LINEAR_ADDR_RECORD = 5,
} hex_record_t;

#pragma pack(push, 1)
typedef struct {
    uint8_t  buf[0x25];
    struct {
        uint8_t  byte_count;
        uint16_t address;
        uint8_t  record_type;
        uint8_t  data[0x25 - 5];
        uint8_t  checksum;
    };
} hex_line_t;
#pragma pack(pop)

static uint16_t swap16(uint16_t a)
{
    return ((a & 0x00ff) << 8) | ((a & 0xff00) >> 8);
}

static uint8_t ctoh(char c)
{
    return (c & 0x10) ? (c & 0xf) : ((c & 0xf) + 9);
}

static uint8_t validate_checksum(hex_line_t *record)
{
    uint8_t result = 0;
    for (uint8_t i = 0; i < (record->byte_count + 5); i++) {
        result += record->buf[i];
    }
    return (result == 0);
}

static hex_line_t line;
static uint32_t next_address_to_write;
static uint8_t low_nibble;
static uint8_t idx;
static uint8_t record_processed;
static uint8_t load_unaligned_record;
static uint8_t binary_version;
static uint8_t skip_until_aligned;

void reset_hex_parser(void)
{
    memset(&line, 0, sizeof(line));
    next_address_to_write = 0;
    low_nibble = 0;
    idx = 0;
    record_processed = 0;
    load_unaligned_record = 0;
    binary_version = 0;
    skip_until_aligned = 0;
}

hexfile_parse_status_t parse_hex_blob(const uint8_t *hex_blob,
    uint32_t hex_blob_size, uint32_t *hex_parse_cnt,
    uint8_t *bin_buf, uint32_t bin_buf_size,
    uint32_t *bin_buf_address, uint32_t *bin_buf_cnt)
{
    const uint8_t *end = hex_blob + hex_blob_size;
    hexfile_parse_status_t status = HEX_PARSE_UNINIT;
    *bin_buf_cnt = 0;

    if (skip_until_aligned) {
        if (hex_blob[0] == ':') {
            skip_until_aligned = 0;
        } else {
            status = HEX_PARSE_OK;
            goto hex_parser_exit;
        }
    }

    if (load_unaligned_record) {
        load_unaligned_record = 0;
        memcpy((uint8_t *)bin_buf, (uint8_t *)line.data, line.byte_count);
        bin_buf += line.byte_count;
        *bin_buf_cnt += line.byte_count;
        next_address_to_write = ((next_address_to_write & 0xffff0000) | line.address) + line.byte_count;
    }

    while (hex_blob != end) {
        switch (*hex_blob) {
        case '\r':
        case '\n':
            break;

        case ':':
            memset(line.buf, 0, sizeof(hex_line_t));
            low_nibble = 0;
            idx = 0;
            record_processed = 0;
            break;

        default:
            if (low_nibble) {
                line.buf[idx] |= ctoh(*hex_blob) & 0xf;
                if (++idx >= (line.byte_count + 5)) {
                    if (0 == validate_checksum(&line)) {
                        return HEX_PARSE_CKSUM_FAIL;
                    }
                    record_processed = 1;
                }
            } else {
                line.buf[idx] = (ctoh(*hex_blob) << 4) & 0xf0;
            }
            low_nibble = !low_nibble;
            break;
        }

        if (record_processed) {
            record_processed = 0;
            switch (line.record_type) {
            case DATA_RECORD:
                if (*bin_buf_cnt == 0) {
                    if (line.address != (next_address_to_write & 0xFFFF)) {
                        return HEX_PARSE_UNALIGNED;
                    }
                } else if (*bin_buf_cnt + line.byte_count > bin_buf_size) {
                    return HEX_PARSE_FAILURE;
                }
                memcpy((uint8_t *)bin_buf, (uint8_t *)line.data, line.byte_count);
                bin_buf += line.byte_count;
                *bin_buf_cnt += line.byte_count;
                next_address_to_write = ((next_address_to_write & 0xffff0000) | line.address) + line.byte_count;
                break;

            case EOF_RECORD:
                *hex_parse_cnt = (uint32_t)(hex_blob - hex_blob + 1);
                return HEX_PARSE_EOF;

            case EXT_SEG_ADDR_RECORD:
                next_address_to_write = ((uint32_t)swap16(*(uint16_t *)line.data)) << 4;
                break;

            case EXT_LINEAR_ADDR_RECORD:
                next_address_to_write = ((uint32_t)swap16(*(uint16_t *)line.data)) << 16;
                break;

            case START_LINEAR_ADDR_RECORD:
                binary_version = 1;
                break;

            case START_SEG_ADDR_RECORD:
                break;

            default:
                return HEX_PARSE_FAILURE;
            }
        }
        hex_blob++;
    }

    *hex_parse_cnt = hex_blob_size;
    if (*bin_buf_cnt == 0) {
        if (skip_until_aligned) {
            status = HEX_PARSE_OK;
        } else {
            status = HEX_PARSE_UNINIT;
        }
    } else {
        status = HEX_PARSE_OK;
    }

hex_parser_exit:
    if (*bin_buf_cnt) {
        *bin_buf_address = next_address_to_write - *bin_buf_cnt;
    } else {
        *bin_buf_address = 0;
    }
    return status;
}
