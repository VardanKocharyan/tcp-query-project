#include <string.h>

#include "protocol/codec.h"
#include "protocol/serializer.h"

protocol_result_info_t serialize_query(
    const Query* query, 
    uint8_t* buffer, 
    size_t capacity
) {
    protocol_result_info_t result;
    result.status = PROTOCOL_INVALID;
    result.bytes_written = 0;
    
    if (
        query == NULL ||
        (query->operation != PING && query->operation != GET && query->operation != SET) ||
        query->key == NULL ||
        query->value == NULL ||
        buffer == NULL
    ) return result;
    

    size_t key_length   = strlen(query->key);  
    size_t value_length = strlen(query->value);
    
    if (key_length > MAX_KEY_SIZE || 
        value_length > MAX_VALUE_SIZE) 
        return result;

    size_t payload_size = 5 + key_length + value_length;
    if (payload_size > MAX_MESSAGE_SIZE) {
        result.status = PROTOCOL_TOO_LARGE;
        return result;
    }

    size_t total_size = payload_size + PROTOCOL_HEADER_SIZE;
    if (total_size > capacity) {
        result.status = PROTOCOL_TOO_LARGE;
        return result;
    }

    buffer[0] = PROTOCOL_MESSAGE_QUERY;
    write_u32(buffer + 1, (uint32_t)payload_size);
    buffer[5] = query->operation;
    write_u16(buffer + 6,(uint16_t)key_length);
    for (size_t i = 0; i < key_length; ++i) 
        buffer[i + 8] = query->key[i];
    write_u16(buffer + 8 + key_length, (uint16_t)value_length);
    for (size_t i = 0; i < value_length; ++i) 
        buffer[i + 10 + key_length] = query->value[i];

    result.status        = PROTOCOL_SUCCESS;
    result.bytes_written = total_size;

    return result;
}
