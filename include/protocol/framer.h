#ifndef FRAMER_H
#define FRAMER_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t* buffer;
    size_t size;
    size_t capacity;
} protocol_framer_t;

void protocol_framer_init(protocol_framer_t* framer);
void protocol_framer_destroy(protocol_framer_t* framer);


#endif 
