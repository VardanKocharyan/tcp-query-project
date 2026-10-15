#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "protocol/framer.h"
#include "protocol/protocol.h"

static void make_header(
    uint8_t* buffer,
    uint8_t message_type,
    uint32_t payload_length
)
{
    buffer[0] = message_type;
    buffer[1] = (uint8_t)(payload_length >> 24);
    buffer[2] = (uint8_t)(payload_length >> 16);
    buffer[3] = (uint8_t)(payload_length >> 8);
    buffer[4] = (uint8_t)payload_length;
}

static void framer_init_test(void)
{
    protocol_framer_t framer;
    protocol_framer_init(&framer);

    assert(framer.buffer == NULL);
    assert(framer.size == 0);
    assert(framer.capacity == 0);

    protocol_framer_destroy(&framer);
}

static void framer_destroy_test(void)
{
    protocol_framer_t framer;
    protocol_framer_init(&framer);

    const uint8_t data[] = {1, 2, 3, 4, 5};

    protocol_framer_info_t result =
        append(&framer, data, sizeof(data));

    assert(result.bytes_consumed == sizeof(data));

    protocol_framer_destroy(&framer);

    assert(framer.buffer == NULL);
    assert(framer.size == 0);
    assert(framer.capacity == 0);
}

static void framer_append_empty_test(void)
{
    protocol_framer_t framer;
    protocol_framer_init(&framer);

    protocol_framer_info_t result = append(&framer, NULL, 0);

    assert(result.status == PROTOCOL_SUCCESS);
    assert(result.bytes_consumed == 0);
    assert(framer.size == 0);

    protocol_framer_destroy(&framer);
}

static void framer_append_invalid_arguments_test(void)
{
    protocol_framer_t framer;
    protocol_framer_init(&framer);

    const uint8_t data[] = {1};

    protocol_framer_info_t result = append(&framer, NULL, 1);

    assert(result.status == PROTOCOL_INVALID);
    assert(result.bytes_consumed == 0);
    assert(framer.size == 0);

    result = append(NULL, data, sizeof(data));

    assert(result.status == PROTOCOL_INVALID);
    assert(result.bytes_consumed == 0);

    protocol_framer_destroy(&framer);
}

static void framer_extract_null_test(void)
{
    protocol_framer_extract_info_t result = protocol_framer_extract(NULL);

    assert(result.status == PROTOCOL_INVALID);
    assert(result.frame == NULL);
    assert(result.frame_size == 0);
}

static void framer_extract_incomplete_header_test(void)
{
    protocol_framer_t framer;
    protocol_framer_init(&framer);

    const uint8_t partial_header[] = {
        PROTOCOL_MESSAGE_QUERY,
        0x00,
        0x00
    };

    protocol_framer_info_t append_result =
        append(&framer, partial_header, sizeof(partial_header));

    assert(append_result.status == PROTOCOL_INCOMPLETE);
    assert(append_result.bytes_consumed == sizeof(partial_header));

    protocol_framer_extract_info_t extract_result = protocol_framer_extract(&framer);

    assert(extract_result.status == PROTOCOL_INCOMPLETE);
    assert(extract_result.frame == NULL);
    assert(extract_result.frame_size == 0);
    assert(framer.size == sizeof(partial_header));

    protocol_framer_destroy(&framer);
}

static void framer_extract_incomplete_payload_test(void)
{
    protocol_framer_t framer;
    protocol_framer_init(&framer);

    const uint8_t frame[] = {
        PROTOCOL_MESSAGE_QUERY,
        0x00, 0x00, 0x00, 0x03,
        'a', 'b', 'c'
    };

    /* Append the header and only one payload byte. */
    protocol_framer_info_t append_result =
        append(&framer, frame, 6);

    assert(append_result.status == PROTOCOL_INCOMPLETE);
    assert(append_result.bytes_consumed == 6);

    protocol_framer_extract_info_t extract_result = protocol_framer_extract(&framer);

    assert(extract_result.status == PROTOCOL_INCOMPLETE);
    assert(extract_result.frame == NULL);
    assert(framer.size == 6);

    /* Append the remaining two payload bytes. */
    append_result = append(&framer, frame + 6, 2);

    assert(append_result.status == PROTOCOL_SUCCESS);
    assert(append_result.bytes_consumed == 2);

    extract_result = protocol_framer_extract(&framer);

    assert(extract_result.status == PROTOCOL_SUCCESS);
    assert(extract_result.frame != NULL);
    assert(extract_result.frame_size == sizeof(frame));
    assert(memcmp(extract_result.frame, frame, sizeof(frame)) == 0);
    assert(framer.size == 0);

    free(extract_result.frame);
    protocol_framer_destroy(&framer);
}

static void framer_extract_complete_frame_test(void)
{
    protocol_framer_t framer;
    protocol_framer_init(&framer);

    const uint8_t frame[] = {
        PROTOCOL_MESSAGE_QUERY,
        0x00, 0x00, 0x00, 0x03,
        'a', 'b', 'c'
    };

    protocol_framer_info_t append_result =
        append(&framer, frame, sizeof(frame));

    assert(append_result.status == PROTOCOL_SUCCESS);
    assert(append_result.bytes_consumed == sizeof(frame));

    protocol_framer_extract_info_t extract_result = protocol_framer_extract(&framer);

    assert(extract_result.status == PROTOCOL_SUCCESS);
    assert(extract_result.frame != NULL);
    assert(extract_result.frame_size == sizeof(frame));
    assert(memcmp(extract_result.frame, frame, sizeof(frame)) == 0);
    assert(framer.size == 0);

    free(extract_result.frame);
    protocol_framer_destroy(&framer);
}

static void framer_extract_consecutive_frames_test(void)
{
    protocol_framer_t framer;
    protocol_framer_init(&framer);

    const uint8_t frame_a[] = {
        PROTOCOL_MESSAGE_QUERY,
        0x00, 0x00, 0x00, 0x02,
        'a', 'b'
    };

    const uint8_t frame_b[] = {
        PROTOCOL_MESSAGE_RESPONSE,
        0x00, 0x00, 0x00, 0x03,
        'x', 'y', 'z'
    };

    uint8_t combined[sizeof(frame_a) + sizeof(frame_b)];

    memcpy(combined, frame_a, sizeof(frame_a));
    memcpy(combined + sizeof(frame_a), frame_b, sizeof(frame_b));

    protocol_framer_info_t append_result =
        append(&framer, combined, sizeof(combined));

    assert(append_result.status == PROTOCOL_SUCCESS);
    assert(append_result.bytes_consumed == sizeof(combined));
    assert(framer.size == sizeof(combined));

    protocol_framer_extract_info_t first = protocol_framer_extract(&framer);

    assert(first.status == PROTOCOL_SUCCESS);
    assert(first.frame_size == sizeof(frame_a));
    assert(memcmp(first.frame, frame_a, sizeof(frame_a)) == 0);
    assert(framer.size == sizeof(frame_b));

    protocol_framer_extract_info_t second = protocol_framer_extract(&framer);

    assert(second.status == PROTOCOL_SUCCESS);
    assert(second.frame_size == sizeof(frame_b));
    assert(memcmp(second.frame, frame_b, sizeof(frame_b)) == 0);
    assert(framer.size == 0);

    free(first.frame);
    free(second.frame);
    protocol_framer_destroy(&framer);
}

static void framer_extract_oversized_payload_test(void)
{
    protocol_framer_t framer;
    protocol_framer_init(&framer);

    uint8_t header[PROTOCOL_HEADER_SIZE];

    make_header(
        header,
        PROTOCOL_MESSAGE_QUERY,
        (uint32_t)MAX_MESSAGE_SIZE + 1U
    );

    protocol_framer_info_t append_result =
        append(&framer, header, sizeof(header));

    assert(append_result.status == PROTOCOL_TOO_LARGE);
    assert(append_result.bytes_consumed == sizeof(header));

    protocol_framer_extract_info_t extract_result = protocol_framer_extract(&framer);

    assert(extract_result.status == PROTOCOL_TOO_LARGE);
    assert(extract_result.frame == NULL);
    assert(extract_result.frame_size == 0);

    protocol_framer_destroy(&framer);
}

static void framer_extract_zero_payload_test(void)
{
    protocol_framer_t framer;
    protocol_framer_init(&framer);

    const uint8_t frame[] = {
        PROTOCOL_MESSAGE_QUERY,
        0x00, 0x00, 0x00, 0x00
    };

    protocol_framer_info_t append_result =
        append(&framer, frame, sizeof(frame));

    assert(append_result.status == PROTOCOL_SUCCESS);
    assert(append_result.bytes_consumed == sizeof(frame));

    protocol_framer_extract_info_t extract_result = protocol_framer_extract(&framer);

    assert(extract_result.status == PROTOCOL_SUCCESS);
    assert(extract_result.frame_size == PROTOCOL_HEADER_SIZE);
    assert(memcmp(extract_result.frame, frame, sizeof(frame)) == 0);
    assert(framer.size == 0);

    free(extract_result.frame);
    protocol_framer_destroy(&framer);
}

int main(void)
{
    framer_init_test();
    framer_destroy_test();
    framer_append_empty_test();
    framer_append_invalid_arguments_test();
    framer_extract_null_test();
    framer_extract_incomplete_header_test();
    framer_extract_incomplete_payload_test();
    framer_extract_complete_frame_test();
    framer_extract_consecutive_frames_test();
    framer_extract_oversized_payload_test();
    framer_extract_zero_payload_test();

    return 0;
}
