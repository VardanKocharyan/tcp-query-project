#ifndef FRAMER_H
#define FRAMER_H

#include <stddef.h>
#include <stdint.h>

#include "protocol/protocol.h"

typedef struct {
    uint8_t* buffer;
    size_t size;
    size_t capacity;
} protocol_framer_t;

void protocol_framer_init(protocol_framer_t* framer);
void protocol_framer_destroy(protocol_framer_t* framer);


protocol_framer_info_t append(
    protocol_framer_t* framer,
    const uint8_t* data,
    size_t size
);

protocol_framer_extract_info_t protocol_framer_extract(
    protocol_framer_t* framer
);

#endif 
