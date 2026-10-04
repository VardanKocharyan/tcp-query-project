#ifndef RESPONSE_SERIALIZER_H
#define RESPONSE_SERIALIZER_H

#include <stdint.h>
#include <stddef.h>

#include "protocol/message.h"
#include "protocol/protocol.h"

protocol_result_info_t serialize_response(
    const Response* response,
    uint8_t* buffer,
    size_t capacity
);

#endif
