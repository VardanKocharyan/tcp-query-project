
#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "protocol/message.h"
#include "protocol/protocol.h"
#include "protocol/codec.h"
#include "protocol/response_serializer.h"

static void test_success_response_exact_bytes(void)
{
    Response response = {
        .status = SUCCESS,
        .value = "hello"
    };

    uint8_t buffer[13] = {0};

    protocol_result_info_t result =
        serialize_response(&response, buffer, sizeof(buffer));

    const uint8_t expected[] = {
        0x02,                   // RESPONSE
        0x00, 0x00, 0x00, 0x08, // Payload length = 8
        0x00,                   // SUCCESS
        0x00, 0x05,             // Value length = 5
        'h', 'e', 'l', 'l', 'o'
    };

    assert(result.status == PROTOCOL_SUCCESS);
    assert(result.bytes_written == sizeof(expected));
    assert(memcmp(buffer, expected, sizeof(expected)) == 0);
}

static void test_fail_response(void)
{
    Response response = {
        .status = FAIL,
        .value = "Not found"
    };

    uint8_t buffer[32] = {0};

    protocol_result_info_t result =
        serialize_response(&response, buffer, sizeof(buffer));

    assert(result.status == PROTOCOL_SUCCESS);
    assert(result.bytes_written == 17);

    assert(buffer[0] == PROTOCOL_MESSAGE_RESPONSE);
    assert(buffer[5] == FAIL);

    assert(read_u16(buffer + 6) == 9);
    assert(memcmp(buffer + 8, "Not found", 9) == 0);
}

static void test_empty_value(void)
{
    Response response = {
        .status = SUCCESS,
        .value = ""
    };

    uint8_t buffer[8] = {0};

    protocol_result_info_t result =
        serialize_response(&response, buffer, sizeof(buffer));

    assert(result.status == PROTOCOL_SUCCESS);
    assert(result.bytes_written == 8);

    assert(buffer[0] == PROTOCOL_MESSAGE_RESPONSE);
    assert(read_u32(buffer + 1) == 3);
    assert(buffer[5] == SUCCESS);
    assert(read_u16(buffer + 6) == 0);
}

static void test_null_response(void)
{
    uint8_t buffer[32];

    protocol_result_info_t result =
        serialize_response(NULL, buffer, sizeof(buffer));

    assert(result.status == PROTOCOL_INVALID);
    assert(result.bytes_written == 0);
}

static void test_null_buffer(void)
{
    Response response = {
        .status = SUCCESS,
        .value = "hello"
    };

    protocol_result_info_t result =
        serialize_response(&response, NULL, 32);

    assert(result.status == PROTOCOL_INVALID);
    assert(result.bytes_written == 0);
}

static void test_null_value(void)
{
    Response response = {
        .status = SUCCESS,
        .value = NULL
    };

    uint8_t buffer[32];

    protocol_result_info_t result =
        serialize_response(&response, buffer, sizeof(buffer));

    assert(result.status == PROTOCOL_INVALID);
    assert(result.bytes_written == 0);
}

static void test_invalid_status(void)
{
    Response response = {
        .status = (query_status_t)100,
        .value = "hello"
    };

    uint8_t buffer[32];

    protocol_result_info_t result =
        serialize_response(&response, buffer, sizeof(buffer));

    assert(result.status == PROTOCOL_INVALID);
    assert(result.bytes_written == 0);
}

static void test_insufficient_capacity(void)
{
    Response response = {
        .status = SUCCESS,
        .value = "hello"
    };

    uint8_t buffer[12];

    protocol_result_info_t result =
        serialize_response(&response, buffer, sizeof(buffer));

    assert(result.status == PROTOCOL_TOO_LARGE);
    assert(result.bytes_written == 0);
}

static void test_max_value_length(void)
{
    char value[MAX_VALUE_SIZE + 1];
    memset(value, 'A', MAX_VALUE_SIZE);
    value[MAX_VALUE_SIZE] = '\0';

    Response response = {
        .status = SUCCESS,
        .value = value
    };

    uint8_t buffer[MAX_VALUE_SIZE + PROTOCOL_HEADER_SIZE + 3];

    protocol_result_info_t result =
        serialize_response(&response, buffer, sizeof(buffer));

    assert(result.status == PROTOCOL_SUCCESS);
    assert(result.bytes_written == MAX_VALUE_SIZE + 8);
    assert(read_u16(buffer + 6) == UINT16_MAX);
}

int main(void)
{
    test_success_response_exact_bytes();
    test_fail_response();
    test_empty_value();

    test_null_response();
    test_null_buffer();
    test_null_value();
    test_invalid_status();

    test_insufficient_capacity();
    test_max_value_length();

    return 0;
}
