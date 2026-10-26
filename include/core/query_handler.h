#ifndef QUERY_HANDLER_H
#define QUERY_HANDLER_H

#include "protocol/message.h"

int query_handler_process(const Query* query, Response* responce);

#endif

