
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include "network/tcp.h"
#include "protocol/message.h"
#include "protocol/protocol.h"
#include "protocol/serializer.h"
#include "protocol/response_deserializer.h"
#include "protocol/framer.h"

#define SERVER_PORT 8080
#define SERVER_ADDRESS "127.0.0.1"

int main(void)
{
    int sockfd = tcp_socket_create();

    if (sockfd < 0) {
        perror("tcp_socket_create");
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_address = {0};
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, SERVER_ADDRESS,
                  &server_address.sin_addr) != 1) {
        fprintf(stderr, "Invalid server address\n");
        close(sockfd);
        return EXIT_FAILURE;
    }

    if (tcp_connect(sockfd, &server_address) != 0) {
        perror("tcp_connect");
        close(sockfd);
        return EXIT_FAILURE;
    }

    Query query;
    query_init(&query, PING, "", "");

    size_t request_capacity = PROTOCOL_HEADER_SIZE + MAX_MESSAGE_SIZE;
    uint8_t *request_buffer = malloc(request_capacity);

    if (request_buffer == NULL) {
        perror("malloc");
        close(sockfd);
        return EXIT_FAILURE;
    }

    protocol_result_info_t serialized =
        serialize_query(&query, request_buffer, request_capacity);

    if (serialized.status != PROTOCOL_SUCCESS) {
        fprintf(stderr, "Failed to serialize query\n");
        free(request_buffer);
        close(sockfd);
        return EXIT_FAILURE;
    }

    tcp_send_info_t sent =
        tcp_send_all(sockfd, request_buffer, serialized.bytes_written);

    free(request_buffer);

    if (sent.status != TCP_SUCCESS ||
        sent.bytes_sent != serialized.bytes_written) {
        fprintf(stderr, "Failed to send complete request\n");
        close(sockfd);
        return EXIT_FAILURE;
    }

    protocol_framer_t framer;
    protocol_framer_init(&framer);

    uint8_t recv_buffer[256];
    int exit_status = EXIT_FAILURE;


    int response_received = 0;

    while (!response_received) {
        tcp_recv_info_t received =
            tcp_recv(sockfd, recv_buffer, sizeof(recv_buffer));

        if (received.status == TCP_DISCONNECTED) {
            fprintf(stderr, "Server disconnected before sending a response\n");
            break;
        }

        if (received.status == TCP_ERROR) {
            fprintf(stderr, "Failed to receive response\n");
            break;
        }

        if (received.byte_received == 0) {
            fprintf(stderr, "No response data received\n");
            break;
        }

        size_t offset = 0;
        int failed = 0;

        while (offset < received.byte_received) {
            protocol_framer_info_t appended =
                append(
                    &framer,
                    recv_buffer + offset,
                    received.byte_received - offset
                );

            if (appended.status != PROTOCOL_SUCCESS &&
                appended.status != PROTOCOL_INCOMPLETE) {
                fprintf(stderr, "Failed to frame response\n");
                failed = 1;
                break;
            }

            if (appended.bytes_consumed == 0) {
                fprintf(stderr, "Framer made no progress\n");
                failed = 1;
                break;
            }

            offset += appended.bytes_consumed;

            while (1) {
                protocol_framer_extract_info_t extracted =
                    protocol_framer_extract(&framer);

                if (extracted.status == PROTOCOL_INCOMPLETE) {
                    break;
                }

                if (extracted.status != PROTOCOL_SUCCESS) {
                    fprintf(stderr, "Failed to extract response frame\n");
                    failed = 1;
                    break;
                }

                Response response = {0};

                protocol_parser_info_t parsed =
                    deserialize_response(
                        extracted.frame,
                        extracted.frame_size,
                        &response
                    );

                free(extracted.frame);

                if (parsed.status != PROTOCOL_SUCCESS ||
                    parsed.bytes_consumed != extracted.frame_size) {
                    fprintf(stderr, "Failed to deserialize response\n");
                    response_free(&response);
                    failed = 1;
                    break;
                }

                printf("Response status: %s\n",
                       response.status == SUCCESS ? "SUCCESS" : "FAIL");
                printf("Response value: %s\n", response.value);

                response_free(&response);

                response_received = 1;
                exit_status = EXIT_SUCCESS;
                break;
            }

            if (failed || response_received) {
                break;
            }
        }

        if (failed) {
            break;
        }
    }

    protocol_framer_destroy(&framer);
    close(sockfd);

    return exit_status;
}

