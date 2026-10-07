#include <stdlib.h>
#include <string.h>

#include "protocol/codec.h"
#include "protocol/response_deserializer.h"

protocol_parser_info_t deserialize_response(
    const uint8_t* buffer,
    size_t capacity,
    Response* response
) {
    protocol_parser_info_t parse = {
        .status = PROTOCOL_INVALID,
        .bytes_consumed = 0
    };

    if (buffer == NULL || response == NULL) {
        return parse;
    }

    if (capacity < PROTOCOL_HEADER_SIZE) {
        parse.status = PROTOCOL_INCOMPLETE;
        return parse;
    }

    if (buffer[0] != PROTOCOL_MESSAGE_RESPONSE) {
        return parse;
    }

    uint32_t payload_length = read_u32(buffer + 1);

    if (payload_length > MAX_MESSAGE_SIZE) {
        parse.status = PROTOCOL_TOO_LARGE;
        return parse;
    }

    /*
     * Response payload:
     *
     * 1 byte  status
     * 2 bytes value length
     * N bytes value
     *
     * Minimum payload = 3 bytes.
     */
    if (payload_length < 3) {
        return parse;
    }

    size_t frame_size = PROTOCOL_HEADER_SIZE + payload_length;

    if (capacity < frame_size) {
        parse.status = PROTOCOL_INCOMPLETE;
        return parse;
    }

    query_status_t status;

    switch (buffer[5]) {
    case SUCCESS:
        status = SUCCESS;
        break;

    case FAIL:
        status = FAIL;
        break;

    default:
        return parse;
    }

    uint16_t value_length = read_u16(buffer + 6);

    /*
     * Value starts at offset 8 and occupies value_length bytes.
     */
    size_t value_end = 8 + (size_t)value_length;

    if (value_end > frame_size) {
        return parse;
    }

    char* value = malloc((size_t)value_length + 1);

    if (value == NULL) {
        return parse;
    }

    memcpy(value, buffer + 8, value_length);
    value[value_length] = '\0';

    /*
     * Only modify the output object after the complete
     * response has been successfully validated and decoded.
     */
    response->status = status;
    response->value = value;

    parse.status = PROTOCOL_SUCCESS;
    parse.bytes_consumed = frame_size;

    return parse;
}
