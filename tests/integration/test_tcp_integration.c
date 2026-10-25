
#define _POSIX_C_SOURCE 200809L

#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <poll.h>

#include "network/tcp.h"
#include "protocol/framer.h"
#include "protocol/message.h"
#include "protocol/protocol.h"
#include "protocol/serializer.h"
#include "protocol/response_deserializer.h"

#define SERVER_PATH "./build/tcp-query-server"

#define CHECK(condition)                                      \
    do {                                                      \
        if (!(condition)) {                                   \
            fprintf(stderr, "FAIL at line %d: %s\n",          \
                    __LINE__, #condition);                    \
            goto cleanup;                                     \
        }                                                     \
    } while (0)

static pid_t start_server(int *port_read_fd, int *port_write_fd)
{
    int pipe_fd[2];

    if (pipe(pipe_fd) == -1) {
        perror("pipe");
        return -1;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        return -1;
    }

    if (pid == 0) {
        close(pipe_fd[0]);

        char port_fd[16];

        (void)snprintf(
            port_fd,
            sizeof(port_fd),
            "%d",
            pipe_fd[1]
        );

        execl(
            SERVER_PATH,
            SERVER_PATH,
            "0",
            port_fd,
            (char *)NULL
        );

        perror("execl");
        _exit(EXIT_FAILURE);
    }

    close(pipe_fd[1]);

    *port_read_fd = pipe_fd[0];
    *port_write_fd = -1;

    return pid;
}

static int connect_to_server(int port)
{
    struct sockaddr_in address = {0};

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons((uint16_t)port);

    for (int attempt = 0; attempt < 100; ++attempt) {
        int fd = tcp_socket_create();

        if (fd < 0) {
            return -1;
        }

        if (tcp_connect(fd, &address) == 0) {
            return fd;
        }

        if (attempt == 99) {
            fprintf(
                stderr,
                "Connect to port %d failed: %s\n",
                port,
                strerror(errno)
            );
        }

        close(fd);

        struct timespec delay = {
            .tv_sec = 0,
            .tv_nsec = 10 * 1000 * 1000
        };

        while (nanosleep(&delay, &delay) == -1 && errno == EINTR) {
            /* Continue sleeping for the remaining interval. */
        }
    }

    return -1;
}

static int set_receive_timeout(int fd)
{
    const struct timeval timeout = {
        .tv_sec = 2,
        .tv_usec = 0
    };

    return setsockopt(
        fd,
        SOL_SOCKET,
        SO_RCVTIMEO,
        &timeout,
        sizeof(timeout)
    );
}

static int read_server_port(int fd)
{
    char buffer[16];
    size_t offset = 0;

    struct timespec deadline;

    if (clock_gettime(CLOCK_MONOTONIC, &deadline) == -1) {
        return -1;
    }

    deadline.tv_sec += 2;

    while (offset < sizeof(buffer) - 1) {
        struct timespec now;

        if (clock_gettime(CLOCK_MONOTONIC, &now) == -1) {
            return -1;
        }

        int64_t remaining_ns =
            (int64_t)(deadline.tv_sec - now.tv_sec) * 1000000000LL +
            (int64_t)deadline.tv_nsec - now.tv_nsec;

        if (remaining_ns <= 0) {
            return -1;
        }

        int timeout_ms = (int)(
            (remaining_ns + 999999LL) / 1000000LL
        );

        struct pollfd poll_fd = {
            .fd = fd,
            .events = POLLIN
        };

        int poll_result = poll(&poll_fd, 1, timeout_ms);

        if (poll_result == -1) {
            if (errno == EINTR) {
                continue;
            }

            return -1;
        }

        if (poll_result == 0) {
            return -1;
        }

        if (poll_fd.revents & (POLLERR | POLLNVAL)) {
            return -1;
        }

        if (!(poll_fd.revents & (POLLIN | POLLHUP))) {
            return -1;
        }

        char ch;
        ssize_t received = read(fd, &ch, 1);

        if (received < 0) {
            if (errno == EINTR) {
                continue;
            }

            return -1;
        }

        if (received == 0) {
            return -1;
        }

        if (ch == '\n') {
            buffer[offset] = '\0';

            char *end = NULL;
            long port = strtol(buffer, &end, 10);

            if (end == buffer ||
                *end != '\0' ||
                port < 1 ||
                port > 65535) {
                return -1;
            }

            return (int)port;
        }

        buffer[offset++] = ch;
    }

    return -1;
}

static int send_fragment(
    int fd,
    const uint8_t *data,
    size_t size
)
{
    tcp_send_info_t result = tcp_send_all(fd, data, size);

    return result.status == TCP_SUCCESS &&
           result.bytes_sent == size;
}

static int stop_server(pid_t pid)
{
    if (pid <= 0) {
        return 1;
    }

    if (kill(pid, SIGTERM) == -1 && errno != ESRCH) {
        perror("kill");
        return 0;
    }

    int status = 0;
    pid_t result;

    do {
        result = waitpid(pid, &status, 0);
    } while (result == -1 && errno == EINTR);

    if (result == -1) {
        perror("waitpid");
        return 0;
    }

    return 1;
}

static int fragmented_ping_request_test(void)
{
    int port_read_fd = -1;
    int port_write_fd = -1;
    int server_port = -1;

    pid_t server_pid = -1;
    int client_fd = -1;
    int framer_initialized = 0;
    int test_result = EXIT_FAILURE;

    protocol_framer_t framer = {0};
    protocol_framer_extract_info_t extracted = {0};
    Response response = {0};

    server_pid = start_server(&port_read_fd, &port_write_fd);
    CHECK(server_pid > 0);

    server_port = read_server_port(port_read_fd);
    CHECK(server_port > 0);

    close(port_read_fd);
    port_read_fd = -1;

    client_fd = connect_to_server(server_port);
    CHECK(client_fd >= 0);

    CHECK(set_receive_timeout(client_fd) == 0);

    Query query = {0};
    query_init(&query, PING, "", "");

    uint8_t request[PROTOCOL_HEADER_SIZE + MAX_MESSAGE_SIZE];

    protocol_result_info_t serialized =
        serialize_query(&query, request, sizeof(request));

    CHECK(serialized.status == PROTOCOL_SUCCESS);
    CHECK(serialized.bytes_written > 4);

    /*
     * Split the request across multiple writes.
     * TCP may combine these writes before the server receives them.
     */
    CHECK(send_fragment(client_fd, request, 2));
    CHECK(send_fragment(client_fd, request + 2, 2));
    CHECK(send_fragment(
        client_fd,
        request + 4,
        serialized.bytes_written - 4
    ));

    protocol_framer_init(&framer);
    framer_initialized = 1;

    uint8_t recv_buffer[256];
    int frame_received = 0;

    for (int attempt = 0; attempt < 100; ++attempt) {
        tcp_recv_info_t received =
            tcp_recv(client_fd, recv_buffer, sizeof(recv_buffer));

        CHECK(received.status == TCP_SUCCESS);
        CHECK(received.byte_received > 0);

        size_t offset = 0;

        while (offset < received.byte_received) {
            protocol_framer_info_t appended = append(
                &framer,
                recv_buffer + offset,
                received.byte_received - offset
            );

            CHECK(
                appended.status == PROTOCOL_SUCCESS ||
                appended.status == PROTOCOL_INCOMPLETE
            );

            CHECK(appended.bytes_consumed > 0);
            offset += appended.bytes_consumed;

            extracted = protocol_framer_extract(&framer);

            if (extracted.status == PROTOCOL_SUCCESS) {
                frame_received = 1;
                break;
            }

            CHECK(extracted.status == PROTOCOL_INCOMPLETE);
        }

        if (frame_received) {
            break;
        }
    }

    CHECK(frame_received);

    protocol_parser_info_t parsed = deserialize_response(
        extracted.frame,
        extracted.frame_size,
        &response
    );

    CHECK(parsed.status == PROTOCOL_SUCCESS);
    CHECK(parsed.bytes_consumed == extracted.frame_size);
    CHECK(response.status == SUCCESS);
    CHECK(response.value != NULL);
    CHECK(strcmp(response.value, "PONG") == 0);

    printf("Integration test passed: received PONG\n");
    test_result = EXIT_SUCCESS;

cleanup:
    response_free(&response);
    free(extracted.frame);

    if (framer_initialized) {
        protocol_framer_destroy(&framer);
    }

    if (client_fd >= 0) {
        close(client_fd);
    }

    if (!stop_server(server_pid)) {
        test_result = EXIT_FAILURE;
    }

    if (port_read_fd >= 0) {
        close(port_read_fd);
    }

    if (port_write_fd >= 0) {
        close(port_write_fd);
    }

    return test_result;
}

int main(void)
{
    return fragmented_ping_request_test();
}

