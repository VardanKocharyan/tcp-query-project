#include "codec.h"

void write_u32(uint8_t* buffer, uint32_t value) {
    buffer[0] = (value & 0xFF000000) >> 24;
    buffer[1] = (value & 0x00FF0000) >> 16;
    buffer[2] = (value & 0x0000FF00) >> 8;
    buffer[3] = (value & 0x000000FF);
}

