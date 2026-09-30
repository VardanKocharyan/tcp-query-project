#include <assert.h>
#include <string.h>

#include "protocol/message.h"

static void test_query_init(void)
{
    Query query;

    query_init(&query, SET, "name", "Vardan");

    assert(query.operation == SET);
    assert(strcmp(query.key, "name") == 0);
    assert(strcmp(query.value, "Vardan") == 0);
}

static void test_response_init(void)
{
    Response response;

    response_init(&response, SUCCESS, "PONG");

    assert(response.status == SUCCESS);
    assert(strcmp(response.value, "PONG") == 0);
}


int main(void) {

    test_query_init();
    test_response_init();

    return 0;
}
