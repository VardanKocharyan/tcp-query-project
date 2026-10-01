#ifndef CODEC_H
#define CODEC_H

#include <stdint.h>

void write_u32(uint8_t* buffer, uint32_t value);

uint32_t read_u32(const uint8_t* buffer);

#endif
