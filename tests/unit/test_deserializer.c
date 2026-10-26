
#include <assert.h>
#include <string.h>

#include "protocol/protocol.h"
#include "protocol/deserializer.h"
#include "protocol/serializer.h"
#include "protocol/codec.h"

void test_deserializer_ping(void)
{
    Query original;
    query_init(&original, PING, "name", "Mihrdat");

    uint8_t buffer[64];

    protocol_result_info_t serializer_result =
        serialize_query(&original, buffer, sizeof(buffer));

    assert(serializer_result.status == PROTOCOL_SUCCESS);

    Query parsed = {0};

    protocol_parser_info_t parser_result =
        deserialize_query(
            buffer,
            serializer_result.bytes_written,
            &parsed
        );

    assert(parser_result.status == PROTOCOL_SUCCESS);
    assert(parser_result.bytes_consumed == serializer_result.bytes_written);

    assert(parsed.operation == original.operation);
    assert(strcmp(parsed.key, original.key) == 0);
    assert(strcmp(parsed.value, original.value) == 0);

    query_free(&parsed);
}

void test_deserializer_set(void)
{
    Query original;
    query_init(&original, SET, "name", "Vardan");

    uint8_t buffer[64];

    protocol_result_info_t serializer_result =
        serialize_query(&original, buffer, sizeof(buffer));

    assert(serializer_result.status == PROTOCOL_SUCCESS);
    assert(serializer_result.bytes_written == 20);

    Query parsed = {0};

    protocol_parser_info_t parser_result =
        deserialize_query(
            buffer,
            serializer_result.bytes_written,
            &parsed
        );

    assert(parser_result.status == PROTOCOL_SUCCESS);
    assert(parser_result.bytes_consumed == 20);

    assert(parsed.operation == SET);
    assert(strcmp(parsed.key, "name") == 0);
    assert(strcmp(parsed.value, "Vardan") == 0);

    query_free(&parsed);
}

void test_deserializer_incomplete_header(void)
{
    uint8_t buffer[4] = {0};
    Query parsed = {0};

    protocol_parser_info_t result =
        deserialize_query(buffer, sizeof(buffer), &parsed);

    assert(result.status == PROTOCOL_INCOMPLETE);
    assert(result.bytes_consumed == 0);
}

void test_deserializer_incomplete_frame(void)
{
    Query original;
    query_init(&original, SET, "name", "Vardan");

    uint8_t buffer[64];

    protocol_result_info_t serialized =
        serialize_query(&original, buffer, sizeof(buffer));

    assert(serialized.status == PROTOCOL_SUCCESS);

    Query parsed = {0};

    protocol_parser_info_t result =
        deserialize_query(
            buffer,
            serialized.bytes_written - 1,
            &parsed
        );

    assert(result.status == PROTOCOL_INCOMPLETE);
    assert(result.bytes_consumed == 0);
}

void test_deserializer_invalid_message_type(void)
{
    uint8_t buffer[20] = {0};

    buffer[0] = 0xFF;

    Query parsed = {0};

    protocol_parser_info_t result =
        deserialize_query(buffer, sizeof(buffer), &parsed);

    assert(result.status == PROTOCOL_INVALID);
    assert(result.bytes_consumed == 0);
}

void test_deserializer_invalid_operation(void)
{
    Query original;
    query_init(&original, PING, "name", "Vardan");

    uint8_t buffer[64];

    protocol_result_info_t serialized =
        serialize_query(&original, buffer, sizeof(buffer));

    assert(serialized.status == PROTOCOL_SUCCESS);

    buffer[5] = 0xFF;

    Query parsed = {0};

    protocol_parser_info_t result =
        deserialize_query(
            buffer,
            serialized.bytes_written,
            &parsed
        );

    assert(result.status == PROTOCOL_INVALID);
    assert(result.bytes_consumed == 0);
}

void test_deserializer_too_small_payload(void)
{
    uint8_t buffer[6] = {
        PROTOCOL_MESSAGE_QUERY,
        0x00, 0x00, 0x00, 0x04,
        PING
    };

    Query parsed = {0};

    protocol_parser_info_t result =
        deserialize_query(buffer, sizeof(buffer), &parsed);

    assert(result.status == PROTOCOL_INVALID);
    assert(result.bytes_consumed == 0);
}

void test_deserializer_null_arguments(void)
{
    uint8_t buffer[20] = {0};

    protocol_parser_info_t result =
        deserialize_query(buffer, sizeof(buffer), NULL);

    assert(result.status == PROTOCOL_INVALID);
    assert(result.bytes_consumed == 0);

    result = deserialize_query(NULL, 20, &(Query){0});

    assert(result.status == PROTOCOL_INVALID);
    assert(result.bytes_consumed == 0);
}


void test_deserializer_trailing_payload_bytes(void)
{
    Query original;
    query_init(&original, PING, "name", "Vardan");

    uint8_t buffer[64];

    protocol_result_info_t serialized =
        serialize_query(&original, buffer, sizeof(buffer));

    assert(serialized.status == PROTOCOL_SUCCESS);

    /*
     * Increase the declared payload length by one byte.
     * The protocol header stores this length in big-endian order.
     */
    buffer[4] += 1;
    buffer[serialized.bytes_written] = 0xAA;

    Query parsed = {0};

    protocol_parser_info_t result =
        deserialize_query(
            buffer,
            serialized.bytes_written + 1,
            &parsed
        );

    assert(result.status == PROTOCOL_INVALID);
    assert(result.bytes_consumed == 0);
}

void test_deserializer_invalid_key_length(void)
{
    Query original;
    query_init(&original, PING, "name", "Vardan");

    uint8_t buffer[64];

    protocol_result_info_t serialized =
        serialize_query(&original, buffer, sizeof(buffer));

    assert(serialized.status == PROTOCOL_SUCCESS);

    // The key length is stored in bytes 6 and 7.
    // Claim a key length of 65535 bytes.
    buffer[6] = 0xFF;
    buffer[7] = 0xFF;

    Query parsed = {0};

    protocol_parser_info_t result = deserialize_query(
        buffer,
        serialized.bytes_written,
        &parsed
    );

    assert(result.status == PROTOCOL_INVALID);
    assert(result.bytes_consumed == 0);
}


void test_deserializer_invalid_value_length(void)
{
    Query original;
    query_init(&original, SET, "name", "Vardan");

    uint8_t buffer[64];

    protocol_result_info_t serialized =
        serialize_query(&original, buffer, sizeof(buffer));

    assert(serialized.status == PROTOCOL_SUCCESS);

    size_t value_length_offset = 8 + strlen("name");

    // Claim a value length of 65535 bytes.
    buffer[value_length_offset] = 0xFF;
    buffer[value_length_offset + 1] = 0xFF;

    Query parsed = {0};

    protocol_parser_info_t result = deserialize_query(
        buffer,
        serialized.bytes_written,
        &parsed
    );

    assert(result.status == PROTOCOL_INVALID);
    assert(result.bytes_consumed == 0);
}


void test_deserializer_declared_payload_larger_than_input(void)
{
    Query original;
    query_init(&original, SET, "name", "Vardan");

    uint8_t buffer[64];

    protocol_result_info_t serialized =
        serialize_query(&original, buffer, sizeof(buffer));

    assert(serialized.status == PROTOCOL_SUCCESS);

    // Increase the declared payload length by one byte,
    // but do not provide the additional byte.
    buffer[4] += 1;

    Query parsed = {0};

    protocol_parser_info_t result = deserialize_query(
        buffer,
        serialized.bytes_written,
        &parsed
    );

    assert(result.status == PROTOCOL_INCOMPLETE);
    assert(result.bytes_consumed == 0);
}


void test_deserializer_failure_leaves_query_unchanged(void)
{
    uint8_t buffer[128] = {0};

    Query original = {
        .operation = SET,
        .key = "key",
        .value = "value"
    };

    protocol_result_info_t serialized =
        serialize_query(&original, buffer, sizeof(buffer));

    assert(serialized.status == PROTOCOL_SUCCESS);

    /*
     * The value-length field follows:
     * header + operation + key-length + key.
     */
    size_t value_length_offset =
        PROTOCOL_HEADER_SIZE + 1 + sizeof(uint16_t) +
        strlen(original.key);

    write_u16(buffer + value_length_offset, UINT16_MAX);

    Query output = {
        .operation = GET,
        .key = NULL,
        .value = NULL
    };

    protocol_parser_info_t result =
        deserialize_query(buffer, serialized.bytes_written, &output);

    assert(result.status == PROTOCOL_INVALID);
    assert(output.operation == GET);
    assert(output.key == NULL);
    assert(output.value == NULL);
}



int main(void)
{
    test_deserializer_ping();
    test_deserializer_set();
    test_deserializer_incomplete_header();
    test_deserializer_incomplete_frame();
    test_deserializer_invalid_message_type();
    test_deserializer_invalid_operation();
    test_deserializer_too_small_payload();
    test_deserializer_null_arguments();
    test_deserializer_trailing_payload_bytes();
    test_deserializer_invalid_key_length();
    test_deserializer_invalid_value_length();
    test_deserializer_declared_payload_larger_than_input();
    test_deserializer_failure_leaves_query_unchanged();

    return 0;
}
