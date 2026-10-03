#ifndef SERIALIZER_H
#define SERIALIZER_H

#include <stddef.h>
#include <stdint.h>

#include "protocol/message.h"
#include "protocol/protocol.h"

protocol_result_info_t serialize_query(
    const Query* query, 
    uint8_t* buffer, 
    size_t capacity
);

#endif
