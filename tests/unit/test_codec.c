#include <assert.h>

#include "protocol/codec.h"

void test_read_write_u32() {
    uint8_t buffer[4];

    write_u32(&buffer, 0x00FF0000);

    assert(buffer[0] == 0x00);
    assert(buffer[1] == 0xFF);
    assert(buffer[2] == 0x00);
    assert(buffer[3] == 0x00);

    assert(read_u32(buffer) == 0x00FF0000);
}

int main(void) {
    
    test_read_write_u32(); 

    return 0;
}
