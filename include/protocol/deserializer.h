#ifndef DESERIALIZER_H
#define DESERIALIZER_H

#include <stdint.h>

#include "protocol/message.h"
#include "protocol/protocol.h"

/*
 * On success, query receives owned copies of the key and value. The output
 * must be an empty, zero-initialized Query; call query_free before reuse.
 * Embedded NUL bytes in wire strings are rejected because the public fields
 * are C strings.
 */
protocol_parser_info_t deserialize_query(
    const uint8_t* buffer,
    size_t length,
    Query* query
);

#endif
