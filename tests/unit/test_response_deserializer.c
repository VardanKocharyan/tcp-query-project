#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "protocol/codec.h"
#include "protocol/message.h"
#include "protocol/response_deserializer.h"
#include "protocol/response_serializer.h"
#include "protocol/protocol.h"


static void test_serialize_deserialize_success(void)
{
    uint8_t buffer[13];

    Response response;
    response_init(&response, SUCCESS, "hello");

    protocol_result_info_t serialize_result =
        serialize_response(&response, buffer, sizeof(buffer));

    assert(serialize_result.status == PROTOCOL_SUCCESS);
    assert(serialize_result.bytes_written == sizeof(buffer));

    Response copy = {0};

    protocol_parser_info_t parse_result =
        deserialize_response(buffer, sizeof(buffer), &copy);

    assert(parse_result.status == PROTOCOL_SUCCESS);
    assert(parse_result.bytes_consumed == sizeof(buffer));

    assert(copy.status == SUCCESS);
    assert(copy.value != NULL);
    assert(strcmp(copy.value, "hello") == 0);

    response_free(&copy);
}


static void test_serialize_deserialize_fail(void)
{
    uint8_t buffer[17];

    Response response;
    response_init(&response, FAIL, "Not found");

    protocol_result_info_t serialize_result =
        serialize_response(&response, buffer, sizeof(buffer));

    assert(serialize_result.status == PROTOCOL_SUCCESS);
    assert(serialize_result.bytes_written == sizeof(buffer));

    Response copy = {0};

    protocol_parser_info_t parse_result =
        deserialize_response(buffer, sizeof(buffer), &copy);

    assert(parse_result.status == PROTOCOL_SUCCESS);
    assert(parse_result.bytes_consumed == sizeof(buffer));

    assert(copy.status == FAIL);
    assert(copy.value != NULL);
    assert(strcmp(copy.value, "Not found") == 0);

    response_free(&copy);
}


static void test_empty_value(void)
{
    uint8_t buffer[8];

    Response response;
    response_init(&response, SUCCESS, "");

    protocol_result_info_t serialize_result =
        serialize_response(&response, buffer, sizeof(buffer));

    assert(serialize_result.status == PROTOCOL_SUCCESS);
    assert(serialize_result.bytes_written == sizeof(buffer));

    Response copy = {0};

    protocol_parser_info_t parse_result =
        deserialize_response(buffer, sizeof(buffer), &copy);

    assert(parse_result.status == PROTOCOL_SUCCESS);
    assert(parse_result.bytes_consumed == sizeof(buffer));

    assert(copy.status == SUCCESS);
    assert(copy.value != NULL);
    assert(copy.value[0] == '\0');

    response_free(&copy);
}


static void test_null_arguments(void)
{
    Response response = {0};
    uint8_t buffer[8] = {0};

    protocol_parser_info_t result;

    result = deserialize_response(NULL, sizeof(buffer), &response);
    assert(result.status == PROTOCOL_INVALID);
    assert(result.bytes_consumed == 0);

    result = deserialize_response(buffer, sizeof(buffer), NULL);
    assert(result.status == PROTOCOL_INVALID);
    assert(result.bytes_consumed == 0);

    result = deserialize_response(NULL, 0, NULL);
    assert(result.status == PROTOCOL_INVALID);
    assert(result.bytes_consumed == 0);
}


static void test_incomplete_header(void)
{
    uint8_t buffer[4] = {0};

    protocol_parser_info_t result =
        deserialize_response(buffer, sizeof(buffer), &(Response){0});

    assert(result.status == PROTOCOL_INCOMPLETE);
    assert(result.bytes_consumed == 0);
}


static void test_wrong_message_type(void)
{
    uint8_t buffer[13] = {0};

    buffer[0] = PROTOCOL_MESSAGE_QUERY;

    protocol_parser_info_t result =
        deserialize_response(buffer, sizeof(buffer), &(Response){0});

    assert(result.status == PROTOCOL_INVALID);
    assert(result.bytes_consumed == 0);
}


static void test_payload_too_large(void)
{
    uint8_t buffer[5] = {0};

    buffer[0] = PROTOCOL_MESSAGE_RESPONSE;
    write_u32(buffer + 1, MAX_MESSAGE_SIZE + 1);

    protocol_parser_info_t result =
        deserialize_response(buffer, sizeof(buffer), &(Response){0});

    assert(result.status == PROTOCOL_TOO_LARGE);
    assert(result.bytes_consumed == 0);
}


static void test_payload_too_small(void)
{
    uint8_t buffer[7] = {0};

    buffer[0] = PROTOCOL_MESSAGE_RESPONSE;
    write_u32(buffer + 1, 2);

    protocol_parser_info_t result =
        deserialize_response(buffer, sizeof(buffer), &(Response){0});

    assert(result.status == PROTOCOL_INVALID);
    assert(result.bytes_consumed == 0);
}


static void test_incomplete_frame(void)
{
    uint8_t buffer[8] = {0};

    buffer[0] = PROTOCOL_MESSAGE_RESPONSE;
    write_u32(buffer + 1, 3);

    buffer[5] = SUCCESS;
    write_u16(buffer + 6, 0);

    /*
     * Header says the frame is 8 bytes, but only 7 are available.
     */
    protocol_parser_info_t result =
        deserialize_response(buffer, 7, &(Response){0});

    assert(result.status == PROTOCOL_INCOMPLETE);
    assert(result.bytes_consumed == 0);
}


static void test_invalid_status(void)
{
    uint8_t buffer[8] = {0};

    buffer[0] = PROTOCOL_MESSAGE_RESPONSE;
    write_u32(buffer + 1, 3);

    buffer[5] = 0xFF;
    write_u16(buffer + 6, 0);

    protocol_parser_info_t result =
        deserialize_response(buffer, sizeof(buffer), &(Response){0});

    assert(result.status == PROTOCOL_INVALID);
    assert(result.bytes_consumed == 0);
}


static void test_invalid_value_length(void)
{
    uint8_t buffer[8] = {0};

    buffer[0] = PROTOCOL_MESSAGE_RESPONSE;
    write_u32(buffer + 1, 3);

    buffer[5] = SUCCESS;

    /*
     * Payload contains only the status and value_length field,
     * but claims that 5 value bytes follow.
     */
    write_u16(buffer + 6, 5);

    protocol_parser_info_t result =
        deserialize_response(buffer, sizeof(buffer), &(Response){0});

    assert(result.status == PROTOCOL_INVALID);
    assert(result.bytes_consumed == 0);
}


static void test_maximum_value(void)
{
    const size_t value_length = MAX_VALUE_SIZE;
    const size_t frame_size = PROTOCOL_HEADER_SIZE + 3 + value_length;

    uint8_t *buffer = malloc(frame_size);
    assert(buffer != NULL);

    Response response;
    char *value = malloc(value_length + 1);
    assert(value != NULL);

    memset(value, 'A', value_length);
    value[value_length] = '\0';

    response_init(&response, SUCCESS, value);

    protocol_result_info_t serialize_result =
        serialize_response(&response, buffer, frame_size);

    assert(serialize_result.status == PROTOCOL_SUCCESS);
    assert(serialize_result.bytes_written == frame_size);

    Response copy = {0};

    protocol_parser_info_t parse_result =
        deserialize_response(buffer, frame_size, &copy);

    assert(parse_result.status == PROTOCOL_SUCCESS);
    assert(parse_result.bytes_consumed == frame_size);
    assert(copy.status == SUCCESS);
    assert(copy.value != NULL);
    assert(strlen(copy.value) == value_length);
    assert(strcmp(copy.value, value) == 0);

    response_free(&copy);
    free(value);
    free(buffer);
}


int main(void)
{
    test_serialize_deserialize_success();
    test_serialize_deserialize_fail();
    test_empty_value();
    test_null_arguments();
    test_incomplete_header();
    test_wrong_message_type();
    test_payload_too_large();
    test_payload_too_small();
    test_incomplete_frame();
    test_invalid_status();
    test_invalid_value_length();
    test_maximum_value();

    return 0;
}

