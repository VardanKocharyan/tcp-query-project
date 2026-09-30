#include <assert.h>
#include <string.h>

#include "core/query_handler.h"

static void test_ping(void)
{
    Query query;
    Response response;

    query_init(&query, PING, NULL, NULL);

    int result = query_handler_process(&query, &response);

    assert(result == 0);
    assert(response.status == SUCCESS);
    assert(strcmp(response.value, "PONG") == 0);
}

static void test_unsupported_operation(void)
{
    Query query;
    Response response;

    query_init(&query, GET, "name", NULL);

    int result = query_handler_process(&query, &response);

    assert(result == 0);
    assert(response.status == FAIL);
    assert(strcmp(response.value, "unsupported operation") == 0);
}

static void test_null_query(void) {
    Response response;

    int result = query_handler_process(NULL, &response);

    assert(result == -1);
}

static void test_null_response(void) {
    Query query;

    query_init(&query, PING, NULL, NULL);

    int result = query_handler_process(&query, NULL);

    assert(result == -1);
}

int main(void)
{
    test_ping();
    test_unsupported_operation();
    test_null_query();
    test_null_query();
    test_null_response();

    return 0;
}
