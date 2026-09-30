#ifndef MESSAGE_H
#define MESSAGE_H

// Query
typedef enum {
    PING,
    GET,
    SET
} query_operation_t;

typedef struct {
    query_operation_t operation;
    const char* key;
    const char* value;

} Query;

void query_init(Query* query, query_operation_t op, 
        const char* k, const char* val);


// Response
typedef enum {
    SUCCESS,
    FAIL
} query_status_t;

typedef struct {
    query_status_t status;
    const char* value;
} Response;

void response_init(Response *response,
                   query_status_t status,
                   const char *value);

#endif
