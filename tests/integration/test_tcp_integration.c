#define _POSIX_C_SOURCE 200809L

#include <arpa/inet.h>
#include <assert.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "network/tcp.h"
#include "protocol/framer.h"
#include "protocol/message.h"
#include "protocol/protocol.h"
#include "protocol/serializer.h"
#include "protocol/response_deserializer.h"

#define TEST_PORT 19081
#define SERVER_PATH "./build/tcp-query-server"

static pid_t start_server(void)
{
    pid_t pid = fork();
    assert(pid >= 0);

    if (pid == 0) {
        char port[16];
        (void)snprintf(port, sizeof(port), "%d", TEST_PORT);
        execl(SERVER_PATH, SERVER_PATH, port, (char *)NULL);
        perror("execl");
        _exit(EXIT_FAILURE);
    }

    return pid;
}

static int connect_to_server(void)
{
    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(TEST_PORT);

    for (int attempt = 0; attempt < 100; ++attempt) {
        int fd = tcp_socket_create();
        assert(fd >= 0);

        if (tcp_connect(fd, &address) == 0) {
            return fd;
        }

        close(fd);

        struct timespec delay = {
            .tv_sec = 0,
            .tv_nsec = 10 * 1000 * 1000
        };
        (void)nanosleep(&delay, NULL);
    }

    return -1;
}

static void send_fragment(int fd, const uint8_t *data, size_t size)
{
    tcp_send_info_t result = tcp_send_all(fd, data, size);
    assert(result.status == TCP_SUCCESS);
    assert(result.bytes_sent == size);
}

static void fragmented_ping_request_test(void)
{
    pid_t server_pid = start_server();

    int client_fd = connect_to_server();
    assert(client_fd >= 0);

    Query query = {0};
    query_init(&query, PING, "", "");

    uint8_t request[PROTOCOL_HEADER_SIZE + MAX_MESSAGE_SIZE];
    protocol_result_info_t serialized =
        serialize_query(&query, request, sizeof(request));

    assert(serialized.status == PROTOCOL_SUCCESS);
    assert(serialized.bytes_written > 3);

    /*
     * Send the frame in multiple pieces:
     *  - first 2 bytes of the header
     *  - next 2 bytes of the header
     *  - remaining bytes
     */
    send_fragment(client_fd, request, 2);
    send_fragment(client_fd, request + 2, 2);
    send_fragment(
        client_fd,
        request + 4,
        serialized.bytes_written - 4
    );

    protocol_framer_t framer;
    protocol_framer_init(&framer);

    uint8_t recv_buffer[256];
    protocol_framer_extract_info_t extracted = {0};

    for (int attempt = 0; attempt < 100; ++attempt) {
        tcp_recv_info_t received =
            tcp_recv(client_fd, recv_buffer, sizeof(recv_buffer));

        assert(received.status == TCP_SUCCESS);
        assert(received.byte_received > 0);

        protocol_framer_info_t appended =
            append(&framer, recv_buffer, received.byte_received);

        assert(appended.status == PROTOCOL_SUCCESS ||
               appended.status == PROTOCOL_INCOMPLETE);

        extracted = protocol_framer_extract(&framer);

        if (extracted.status == PROTOCOL_SUCCESS) {
            break;
        }

        assert(extracted.status == PROTOCOL_INCOMPLETE);
    }

    assert(extracted.status == PROTOCOL_SUCCESS);

    Response response = {0};
    protocol_parser_info_t parsed =
        deserialize_response(
            extracted.frame,
            extracted.frame_size,
            &response
        );

    assert(parsed.status == PROTOCOL_SUCCESS);
    assert(parsed.bytes_consumed == extracted.frame_size);
    assert(response.status == SUCCESS);
    assert(strcmp(response.value, "PONG") == 0);

    response_free(&response);
    free(extracted.frame);
    protocol_framer_destroy(&framer);
    close(client_fd);

    assert(kill(server_pid, SIGTERM) == 0);

    int status = 0;
    assert(waitpid(server_pid, &status, 0) == server_pid);
    assert(WIFSIGNALED(status));
    assert(WTERMSIG(status) == SIGTERM);
}

int main(void)
{
    fragmented_ping_request_test();
    return 0;
}

