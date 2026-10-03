#ifndef DESERIALIZER_H
#define DESERIALIZER_H

#include <stdint.h>

#include "protocol/message.h"
#include "protocol/protocol.h"

protocol_parser_info_t deserialize_query(
    const uint8_t* buffer,
    size_t length,
    Query* query
);

#endif
