#include <core/query_handler.h>
#include <stddef.h>

int query_handler_process(const Query *query, Response *response)
{
    if (query == NULL || response == NULL) {
        return -1;
    }

    switch (query->operation) {
        case PING:
            response_init(response, SUCCESS, "PONG");
            return 0;

        case GET:
        case SET:
            if (query->key == NULL) {
                return -1;
            }

            if (query->key[0] == '\0') {
                response_init(response, FAIL, "empty key");
                return 0;
            }

            response_init(response, FAIL, "unsupported operation");
            return 0;

        default:
            response_init(response, FAIL, "invalid operation");
            return -1;
    }
}
