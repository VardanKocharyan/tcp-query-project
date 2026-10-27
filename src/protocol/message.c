#include <stdlib.h>

#include "protocol/message.h"

void query_init(Query* query,
                query_operation_t op,
                const char* k,
                const char* val) 
{
    query->operation    = op;
    query->key          = k;
    query->value        = val;

    query->owns_key = false;
    query->owns_value = false;
}

void response_init(Response *response,
                   query_status_t status,
                   const char *value)
{
    response->status = status;
    response->value = value;

    response->owns_value = false;
}

void query_free(Query* query)
{
    if (query == NULL) {
        return;
    }

    if (query->owns_key) {
        free((void*)query->key);
    }

    if (query->owns_value) {
        free((void*)query->value);
    }

    query->key = NULL;
    query->value = NULL;
    query->owns_key = false;
    query->owns_value = false;
}

void response_free(Response* response)
{
    if (response == NULL) {
        return;
    }

    if (response->owns_value) {
        free((void*)response->value);
    }

    response->value = NULL;
    response->owns_value = false;
}
