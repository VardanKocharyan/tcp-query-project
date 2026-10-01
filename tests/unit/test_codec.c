#include <assert.h>

#include "protocol/codec.h"

void test_read_write_u16(void)
{
    uint8_t buffer[2];

    write_u16(buffer, 0x1234);

    assert(buffer[0] == 0x12);
    assert(buffer[1] == 0x34);

    assert(read_u16(buffer) == 0x1234);
}

void test_read_write_u32() {
    uint8_t buffer[4];

    write_u32(buffer, 0x00FF0000);

    assert(buffer[0] == 0x00);
    assert(buffer[1] == 0xFF);
    assert(buffer[2] == 0x00);
    assert(buffer[3] == 0x00);

    assert(read_u32(buffer) == 0x00FF0000);
}

void test_read_write_u64(void)
{
    uint8_t buffer[8];

    write_u64(buffer, 0x123456789ABCDEF0ULL);

    assert(buffer[0] == 0x12);
    assert(buffer[1] == 0x34);
    assert(buffer[2] == 0x56);
    assert(buffer[3] == 0x78);
    assert(buffer[4] == 0x9A);
    assert(buffer[5] == 0xBC);
    assert(buffer[6] == 0xDE);
    assert(buffer[7] == 0xF0);

    assert(read_u64(buffer) == 0x123456789ABCDEF0ULL);
}

int main(void) {
 
    test_read_write_u16();
    test_read_write_u32();
    test_read_write_u64();

    return 0;
}
