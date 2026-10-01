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

void write_u16(uint8_t* buffer, uint16_t value) {
    buffer[0] = (value & 0xFF00) >> 8;
    buffer[1] = (value & 0x00FF);
}

uint16_t read_u16(const uint8_t* buffer) {
    uint16_t value = (
        ((uint16_t)buffer[0] << 8) |
        ((uint16_t)buffer[1])
    );
    return value;
}

void write_u64(uint8_t* buffer, uint64_t value) {
    buffer[0] = (value & 0xFF00000000000000) >> 56;
    buffer[1] = (value & 0x00FF000000000000) >> 48;
    buffer[2] = (value & 0x0000FF0000000000) >> 40;
    buffer[3] = (value & 0x000000FF00000000) >> 32;
    buffer[4] = (value & 0x00000000FF000000) >> 24;
    buffer[5] = (value & 0x0000000000FF0000) >> 16;
    buffer[6] = (value & 0x000000000000FF00) >> 8;
    buffer[7] = (value & 0x00000000000000FF);
}

uint64_t read_u64(const uint8_t* buffer) {
    uint64_t value = (
        ((uint64_t)buffer[0] << 56) |
        ((uint64_t)buffer[1] << 48) |
        ((uint64_t)buffer[2] << 40) |
        ((uint64_t)buffer[3] << 32) |
        ((uint64_t)buffer[4] << 24) |
        ((uint64_t)buffer[5] << 16) |
        ((uint64_t)buffer[6] << 8) |
        ((uint64_t)buffer[7])
    );
    return value;
}
