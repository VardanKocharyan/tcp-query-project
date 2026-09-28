#ifndef MESSAGE_H
#define MESSAGE_H

typedef enum {
    PING,
    GET,
    SET
} query_operation_t;

typedef struct {
    query_operation_t operation;
    const char * key;
    const char* value;

} Query;

void query_init(Query* query, query_operation_t op, 
        const char* k, const char* val);

#endif
