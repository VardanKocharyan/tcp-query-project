#ifndef RESPONSE_DESERIALIZER_H
#define RESPONSE_DESERIALIZER_H

#include <stdint.h>
#include <stddef.h>

#include "protocol/message.h"
#include "protocol/protocol.h"

protocol_parser_info_t deserialize_response(
    const uint8_t* buffer,
    size_t capacity,
    Response* response
);

#endif
