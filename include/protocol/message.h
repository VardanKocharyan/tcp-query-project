#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdbool.h>

// Query: query_init stores borrowed strings; query_free only frees fields
// marked as owned. Do not reinitialize an object while it owns allocations.
typedef enum {
    PING,
    GET,
    SET
} query_operation_t;

typedef struct {
    query_operation_t operation;
    const char* key;
    const char* value;

    bool owns_key;
    bool owns_value;
} Query;

void query_init(Query* query, query_operation_t op,
                const char* k, const char* val);

void query_free(Query* query);


// Response: response_init stores a borrowed value; response_free only frees
// the value when owns_value is true. Do not reinitialize an owning object.
typedef enum {
    SUCCESS,
    FAIL
} query_status_t;

typedef struct {
    query_status_t status;
    const char* value;

    bool owns_value;
} Response;

void response_init(Response* response,
                   query_status_t status,
                   const char* value);

void response_free(Response* response);

#endif
