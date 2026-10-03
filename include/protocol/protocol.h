#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stddef.h>

#include "protocol/message.h"

#define PROTOCOL_MESSAGE_QUERY    0x01
#define PROTOCOL_MESSAGE_RESPONSE 0x02

#define MAX_MESSAGE_SIZE  (1024 * 1024) // 1 Mb
#define MAX_KEY_SIZE      UINT16_MAX //65535
#define MAX_VALUE_SIZE    UINT16_MAX //65535

#define PROTOCOL_HEADER_SIZE 5


typedef enum {
    PROTOCOL_SUCCESS,
    PROTOCOL_INCOMPLETE,
    PROTOCOL_INVALID,
    PROTOCOL_TOO_LARGE
} protocol_result_t;


typedef struct {
    protocol_result_t status;
    uint32_t bytes_written;
} protocol_result_info_t;

typedef struct {
    protocol_result_t status;
    size_t bytes_consumed;
} protocol_parser_info_t;

#endif


