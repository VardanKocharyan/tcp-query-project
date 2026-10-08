#include <stdlib.h>

#include "protocol/framer.h"

void protocol_framer_init(protocol_framer_t* framer)
{
    framer->buffer = NULL;
    framer->size = 0;
    framer->capacity = 0;
}

void protocol_framer_destroy(protocol_framer_t* framer)
{
    free(framer->buffer);

    framer->buffer = NULL;
    framer->size = 0;
    framer->capacity = 0;
}
