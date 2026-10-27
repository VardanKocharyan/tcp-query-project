#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
//#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <limits.h>

#include "network/tcp.h"
#include "protocol/framer.h"
#include "core/query_handler.h"
#include "protocol/deserializer.h"
#include "protocol/response_serializer.h"

#define DEFAULT_SERVER_PORT 8080
#define BACKLOG 5

static int process_frame(int client_fd,
                         const uint8_t *frame,
                         size_t frame_size)
{
    Query query = {0};

    protocol_parser_info_t parse_result =
        deserialize_query(frame, frame_size, &query);

    if (parse_result.status != PROTOCOL_SUCCESS ||
        parse_result.bytes_consumed != frame_size) {
        query_free(&query);
        return -1;
    }

    Response response = {0};

    int handler_result = query_handler_process(&query, &response);

    if (handler_result != 0 || response.value == NULL) {
        query_free(&query);
        return -1;
    }

    size_t response_capacity =
        PROTOCOL_HEADER_SIZE + MAX_MESSAGE_SIZE;

    uint8_t *response_buffer = malloc(response_capacity);

    if (response_buffer == NULL) {
        query_free(&query);
        return -1;
    }

    protocol_result_info_t serialize_result =
        serialize_response(
            &response,
            response_buffer,
            response_capacity
        );

    int result = -1;

    if (serialize_result.status == PROTOCOL_SUCCESS) {
        tcp_send_info_t send_result =
            tcp_send_all(
                client_fd,
                response_buffer,
                serialize_result.bytes_written
            );

        if (send_result.status == TCP_SUCCESS &&
            send_result.bytes_sent == serialize_result.bytes_written) {
            result = 0;
        }
    }

    free(response_buffer);
    query_free(&query);

    return result;
}

static int report_port(int fd, int port)
{
    char buffer[16];

    int length = snprintf(buffer, sizeof(buffer), "%d\n", port);

    if (length < 0 || (size_t)length >= sizeof(buffer)) {
        return -1;
    }

    size_t offset = 0;

    while (offset < (size_t)length) {
        ssize_t written = write(
            fd,
            buffer + offset,
            (size_t)length - offset
        );

        if (written < 0) {
            if (errno == EINTR) {
                continue;
            }

            return -1;
        }

        if (written == 0) {
            return -1;
        }

        offset += (size_t)written;
    }

    return 0;
}

int main(int argc, char* argv[])
{
    int port = DEFAULT_SERVER_PORT;

    if (argc > 1) {
        char *end = NULL;
        long parsed_port = strtol(argv[1], &end, 10);

        if (end == argv[1] ||
            *end != '\0' ||
            parsed_port < 0 ||
            parsed_port > 65535) {
            fprintf(stderr, "Invalid port\n");
            return 1;
        }

        port = (int)parsed_port;
    }

    int report_fd = -1;

    if (argc > 2) {
        char *end = NULL;
        long parsed_fd = strtol(argv[2], &end, 10);

        if (end == argv[2] ||
            *end != '\0' ||
            parsed_fd < 0 ||
            parsed_fd > INT_MAX) {
            fprintf(stderr, "Invalid port reporting descriptor\n");
            return 1;
        }
    
        report_fd = (int)parsed_fd;
    }

    int server_fd = tcp_socket_create();
    if (server_fd < 0) {
        perror("tcp_socket_create");
        return 1;
    }

    struct sockaddr_in server_address = {0};
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons((uint16_t)port);

    if (bind(
            server_fd,
            (struct sockaddr *)&server_address,
            sizeof(server_address)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (getsockname(
            server_fd,
            (struct sockaddr *)&server_address,
            &(socklen_t){sizeof(server_address)}) < 0) {
        perror("getsockname");
        close(server_fd);
        return 1;
    }

    port = (int)ntohs(server_address.sin_port);


    if (listen(server_fd, BACKLOG) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    if (report_fd >= 0) {
        if (report_port(report_fd, port) != 0) {
            perror("report_port");
            close(report_fd);
            close(server_fd);
            return 1;
        }

        close(report_fd);
    }

    printf("Server listening on port %d\n", (uint16_t)port);
    fflush(stdout);

    uint8_t buffer[256];

    for (;;) {
        struct sockaddr_in client_address = {0};
        socklen_t client_address_len = sizeof(client_address);

        int client_fd = accept(
            server_fd,
            (struct sockaddr *)&client_address,
            &client_address_len);

        if (client_fd < 0) {
            if (errno == EINTR) {
                continue;
            }

            perror("accept");
            close(server_fd);
            return EXIT_FAILURE;
        }

        printf("Client connected\n");

        protocol_framer_t framer;
        protocol_framer_init(&framer);

        while (true) {
            tcp_recv_info_t recv_result =
                tcp_recv(client_fd, buffer, sizeof(buffer));

            if (recv_result.status == TCP_DISCONNECTED) {
                break;
            }

            if (recv_result.status == TCP_ERROR) {
                perror("recv");
                break;
            }

            size_t offset = 0;

            while (offset < recv_result.byte_received) {
                protocol_framer_info_t append_result =
                    append(
                        &framer,
                        buffer + offset,
                        recv_result.byte_received - offset
                    );

                if (append_result.status != PROTOCOL_SUCCESS &&
                    append_result.status != PROTOCOL_INCOMPLETE) {
                    fprintf(stderr, "Failed to append incoming data\n");
                    goto client_done;
                }

                if (append_result.bytes_consumed == 0) {
                    fprintf(stderr, "Framer made no progress\n");
                    goto client_done;
                }

                offset += append_result.bytes_consumed;

                while (true) {
                    protocol_framer_extract_info_t extract_result =
                        protocol_framer_extract(&framer);

                    if (extract_result.status == PROTOCOL_INCOMPLETE) {
                        break;
                    }

                    if (extract_result.status != PROTOCOL_SUCCESS) {
                        fprintf(stderr, "Failed to extract frame\n");
                        goto client_done;
                    }

                    int process_result =
                        process_frame(
                            client_fd,
                            extract_result.frame,
                            extract_result.frame_size
                        );

                    free(extract_result.frame);

                    if (process_result != 0) {
                        fprintf(stderr, "Failed to process request\n");
                        goto client_done;
                    }
                }
            }
        }
client_done:
        protocol_framer_destroy(&framer); 
        close(client_fd);
    }

    close(server_fd);
    return 0;
}
