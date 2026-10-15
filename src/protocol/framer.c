#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "protocol/protocol.h"
#include "protocol/framer.h"
#include "protocol/codec.h"

void protocol_framer_init(protocol_framer_t* framer)
{
    framer->buffer = NULL;
    framer->size = 0;
    framer->capacity = 0;
}

void protocol_framer_destroy(protocol_framer_t* framer)
{
    free(framer->buffer);

    framer->buffer = NULL;
    framer->size = 0;
    framer->capacity = 0;
}

protocol_framer_info_t append(
    protocol_framer_t* framer,
    const uint8_t* data,
    size_t size
) {
    protocol_framer_info_t result = {
        .status = PROTOCOL_INVALID, 
        .bytes_consumed = 0
    };

    if (
        framer == NULL || 
        (data == NULL && size != 0) ||
        framer->size > framer->capacity ||
        (framer->capacity != 0 && framer->buffer == NULL)
    ) {
        return result;
    }

    if (size == 0) {
        result.status = PROTOCOL_SUCCESS;
        return result;
    }

    const size_t max_frame_size = 
        PROTOCOL_HEADER_SIZE + (size_t)MAX_MESSAGE_SIZE;

    if (framer->size >= max_frame_size) {
        result.status = PROTOCOL_TOO_LARGE;
        return result;
    }

    size_t available = max_frame_size  - framer->size;
    size_t bytes_to_copy = size < available ? size : available;

    size_t required_size = framer->size + bytes_to_copy;

    if (framer->capacity < required_size) {
        uint8_t* tmp = realloc(framer->buffer, required_size);

        if (tmp == NULL) {
            return result;
        }

        framer->buffer = tmp;
        framer->capacity = required_size;
    }

    memcpy(
        framer->buffer,
        data,
        bytes_to_copy
    );

    framer->size += bytes_to_copy;
    result.bytes_consumed = bytes_to_copy;

    if (framer->size >= PROTOCOL_HEADER_SIZE) {
        uint32_t payload_length = read_u32(framer->buffer + 1);

        if (payload_length > MAX_MESSAGE_SIZE) {
            result.status = PROTOCOL_TOO_LARGE;
            return result;
        }

        size_t frame_size =
            PROTOCOL_HEADER_SIZE + (size_t)payload_length;

        if (framer->size >= frame_size) {
            result.status = PROTOCOL_SUCCESS;
            return result;
        }
    }

    result.status = PROTOCOL_INCOMPLETE;
    return result;
}

protocol_framer_extract_info_t protocol_framer_extract(
    protocol_framer_t* framer
) {
    protocol_framer_extract_info_t result = {
        .status = PROTOCOL_INVALID,
        .frame = NULL,
        .frame_size = 0
    };

    if (
        framer == NULL ||
        (framer->size != 0 && framer->buffer == NULL) ||
        framer->size > framer->capacity
    ) {
        return result;
    }

    if (framer->size < PROTOCOL_HEADER_SIZE) {
        result.status = PROTOCOL_INCOMPLETE;
        return result;
    }

    uint32_t payload_length = read_u32(framer->buffer + 1);

    if (payload_length > MAX_MESSAGE_SIZE) {
        result.status = PROTOCOL_TOO_LARGE;
        return result;
    }

    size_t frame_size = 
        PROTOCOL_HEADER_SIZE + (size_t)payload_length;

    if (framer->size < frame_size) {
        result.status = PROTOCOL_INCOMPLETE;
        return result;
    }

    result.frame = malloc(frame_size);

    if (result.frame == NULL) {
        return result;
    }

    memcpy(
        result.frame, 
        framer->buffer, 
        frame_size
    );

    size_t remaining_size = framer->size - frame_size;

    memmove(
        framer->buffer, 
        framer->buffer + frame_size, 
        remaining_size
    );

    framer->size = remaining_size;
 
    result.status = PROTOCOL_SUCCESS;
    result.frame_size = frame_size;
    return result;
}
