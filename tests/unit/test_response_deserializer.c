#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "protocol/message.h"
#include "protocol/response_serializer.h"
#include "protocol/response_deserializer.h"


void test_serialize_deserialize_success(void)
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

    assert(copy.status == response.status);
    assert(copy.value != NULL);
    assert(strcmp(copy.value, response.value) == 0);

    response_free(&copy);
}

int main(void)
{
    test_serialize_deserialize_success();

    return 0;
}

