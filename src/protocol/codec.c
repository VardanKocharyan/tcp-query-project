#include "protocol/codec.h"

void write_u32(uint8_t* buffer, uint32_t value) {
    buffer[0] = (value & 0xFF000000) >> 24;
    buffer[1] = (value & 0x00FF0000) >> 16;
    buffer[2] = (value & 0x0000FF00) >> 8;
    buffer[3] = (value & 0x000000FF);
}

uint32_t read_u32(const uint8_t* buffer) {
    uint32_t value = (
        ((uint32_t)buffer[0] << 24) |
        ((uint32_t)buffer[1] << 16) |
        ((uint32_t)buffer[2] << 8)  |
        (uint32_t)buffer[3]
    );
    return value;
}
