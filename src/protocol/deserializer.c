#include "protocol/deserializer.h"
#include "protocol/codec.h"

protocol_parser_info_t deserialize_query(
    const uint8_t* buffer,
    size_t length,
    Query* query
) {
    protocol_parser_info_t parse = {
        .status = PROTOCOL_INVALID;
        .bytes_consumed = 0;
    };

    if (buffer == NULL || query == NULL) {
        return parse;
    }

    if (length < PROTOCOL_HEADER_SIZE) {
        parse.status = PROTOCOL_INCOMPLETE;
        return parse;
    }

    if (buffer[0] != PROTOCOL_MESSAGE_QUERY) {
        return parse;
    }

    uint32_t paylure_length = read_u32(buffer + 1);
    
    if (paylure_length > MAX_MESSAGE_SIZE) {
        parse.status = PROTOCOL_TOO_LARGE;
        return parse;
    }

    if (paylure_length < 5) {
        return parse;
    }

    size_t frame_size = PROTOCOL_HEADER_SIZE + payload_length;

    if (length < frame_size) {
        parse.status = PROTOCOL_INCOMPLETE;
        return parse;
    }

    switch (buffer[5]) {
        case PING:
            query->operation = PING;
            break;

        case GET:
            query->operation = GET;
            break;

        case SET:
            query->operation = SET;
            break;

        default:
            return parse;
    }

    uint16_t key_length = read_u16(buffer + 6);

    size_t key_start = 8;
    size_t value_length_offset = key_start + key_length;

    if (value_length_offset + sizeof(uint16_t) > frame_size) {
        return parse;
    }

    uint16_t value_length = read_u16(buffer + value_length_offset);
    
    size_t value_start = value_length_offset + sizeof(uint16_t);

    if (value_start + value_length > frame_size) {
        return parse;
    }

    char* key = malloc((size_t)key_length + 1);
    if (key == NULL) {
        return parse;
    }

    char* value = malloc((size_t)value_length + 1);
    if (value == NULL) {
        free(key);
        return parse;
    }

    memcpy(key, buffer + key_start, key_length);
    key[key_length] = '\0';

    memcpy(value, buffer + value_start, value_length);
    value[value_length] = '\0';

    query->key = key;
    query->value = value;

    parse.status = PROTOCOL_SUCCESS;
    parse.bytes_consumed = frame_size;

    return parse;
}
