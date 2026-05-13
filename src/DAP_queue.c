#include "DAP_queue.h"
#include "DAP_config.h"
#include <string.h>

#define DAP_QUEUE_ELEMENT_SIZE (DAP_PACKET_SIZE + 4)
#define DAP_QUEUE_ELEMENT_COUNT DAP_PACKET_COUNT

static uint8_t queue_buffer[DAP_QUEUE_ELEMENT_SIZE * DAP_QUEUE_ELEMENT_COUNT];
static uint8_t queue_element_len[DAP_QUEUE_ELEMENT_COUNT];
static int qhead, qtail;

void DAP_queue_init(void)
{
    qhead = 0;
    qtail = 0;
}

int DAP_queue_send_buf(uint8_t *buf, int len)
{
    int next = (qhead + 1) % DAP_QUEUE_ELEMENT_COUNT;
    if (next == qtail) {
        return 0;
    }
    memcpy(&queue_buffer[qhead * DAP_QUEUE_ELEMENT_SIZE], buf, len);
    queue_element_len[qhead] = len;
    qhead = next;
    return 1;
}

int DAP_queue_get_buf(uint8_t **buf, int *len)
{
    if (qhead == qtail) {
        return 0;
    }
    *buf = &queue_buffer[qtail * DAP_QUEUE_ELEMENT_SIZE];
    *len = queue_element_len[qtail];
    qtail = (qtail + 1) % DAP_QUEUE_ELEMENT_COUNT;
    return 1;
}
