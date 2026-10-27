#ifndef RESPONSE_DESERIALIZER_H
#define RESPONSE_DESERIALIZER_H

#include <stdint.h>
#include <stddef.h>

#include "protocol/message.h"
#include "protocol/protocol.h"

/*
 * On success, response receives an owned copy of the value. The output must
 * be an empty, zero-initialized Response; call response_free before reuse.
 * Embedded NUL bytes and trailing payload bytes are rejected.
 */
protocol_parser_info_t deserialize_response(
    const uint8_t* buffer,
    size_t capacity,
    Response* response
);

#endif
