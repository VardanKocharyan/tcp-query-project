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
}

void response_init(Response *response,
                   query_status_t status,
                   const char *value)
{
    response->status = status;
    response->value = value;
}

void query_free(Query* query)
{
    if (query == NULL) {
        return;
    }

    free((void*)query->key);
    free((void*)query->value);

    query->key = NULL;
    query->value = NULL;
}

void response_free(Response* response)
{
    if (response == NULL) {
        return;
    }

    free((void*)response->value);
    response->value = NULL;
}
