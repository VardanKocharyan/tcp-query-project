#ifndef CODEC_H
#define CODEC_H

#include <stdint.h>

void write_u32(uint8_t* buffer, uint32_t value);
uint32_t read_u32(const uint8_t* buffer);

void write_u16(uint8_t* buffer, uint16_t value);
uint16_t read_u16(const uint8_t* buffer);

void write_u64(uint8_t* buffer, uint64_t value);
uint64_t read_u64(const uint8_t* buffer);

#endif
