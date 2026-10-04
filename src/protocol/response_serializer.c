#include <string.h>

#include "protocol/codec.h"
#include "protocol/response_serializer.h"

protocol_result_info_t serialize_response(
    const Response* response,
    uint8_t* buffer,
    size_t capacity
) {
    protocol_result_info_t result = {
        .status = PROTOCOL_INVALID,
        .bytes_written = 0
    };

    if (
        response == NULL || 
        buffer == NULL ||
        (response->status != SUCCESS && response->status != FAIL) ||
        response->value == NULL
    ) {
        return result;
    }

    size_t value_length = strlen(response->value);

    if (value_length > MAX_VALUE_SIZE) {
        result.status = PROTOCOL_TOO_LARGE;
        return result;
    }

    size_t payload_size = 3 + value_length;

    if (payload_size > MAX_MESSAGE_SIZE) {
        result.status = PROTOCOL_TOO_LARGE;
        return result;
    }

    size_t total_size = PROTOCOL_HEADER_SIZE + payload_size;

    if (capacity < total_size) {
        result.status = PROTOCOL_TOO_LARGE;
        return result;
    }

    buffer[0] = PROTOCOL_MESSAGE_RESPONSE;
    
    write_u32(buffer + 1, (uint32_t)payload_size);
    
    buffer[5] = (uint8_t)response->status;
    
    write_u16(buffer + 6, (uint16_t)value_length);

    for (size_t i = 0; i < value_length; ++i) { 
        buffer[i + 8] = response->value[i];
    }

    result.status = PROTOCOL_SUCCESS;
    result.bytes_written = total_size;

    return result;
}
