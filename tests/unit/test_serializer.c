#include <assert.h>
#include <string.h>

#include "protocol/protocol.h"
#include "protocol/serializer.h"

void test_serializer1(void) {
    Query query;
    query_init(&query, PING, "name", "Mihrdat");

    uint8_t buffer[50];

    protocol_result_info_t result = 
        serialize_query(
            &query,
            buffer,
            50
        );

    assert(result.status == PROTOCOL_SUCCESS);
    assert(result.bytes_written == 21);
}

void test_serializer2(void) {
    Query query;
    query_init(&query, SET, "name", "Vardan");

    uint8_t buffer[30];

    protocol_result_info_t result = 
        serialize_query(
            &query,
            buffer,
            30
        );

    const uint8_t expected[] = {
        0x01,                   // message type
        0x00, 0x00, 0x00, 0x0F, // payload length = 15
        0x02,                   // SET
        0x00, 0x04,             // key length = 4
        'n', 'a', 'm', 'e',
        0x00, 0x06,             // value length = 6
        'V', 'a', 'r', 'd', 'a', 'n'
    };

    assert(result.status == PROTOCOL_SUCCESS);
    assert(result.bytes_written == 20);
    assert(memcmp(buffer, expected, sizeof(expected)) == 0);
}

void test_serializer3(void) {
    Query query;
    query_init(&query, PING, "name", "Antuaneta");

    uint8_t buffer[10];

    protocol_result_info_t result = 
        serialize_query(
            &query,
            buffer,
            10
        );

    assert(result.status == PROTOCOL_TOO_LARGE);
    assert(result.bytes_written == 0);
}


int main(void) {

    test_serializer1();
    test_serializer2();
    test_serializer3();

    return 0;
}
