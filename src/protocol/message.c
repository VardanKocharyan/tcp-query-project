#include "massage.h"

void query_init(Query* query,
                query_operation_t op,
                const char* k,
                const char* val) {
    query->operation    = op;
    query->key          = k;
    query->value        = val;
}
