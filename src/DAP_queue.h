#ifndef DAP_QUEUE_H
#define DAP_QUEUE_H

#include <stdint.h>

void DAP_queue_init(void);
int  DAP_queue_send_buf(uint8_t *buf, int len);
int  DAP_queue_get_buf(uint8_t **buf, int *len);

#endif
