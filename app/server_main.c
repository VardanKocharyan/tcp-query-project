#include <arpa/inet.h>
#include <stdio.h>
//#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>

#include "network/tcp.h"

#define SERVER_PORT 8080
#define BACKLOG 5

int main(void)
{
    int server_fd = tcp_socket_create();
    if (server_fd < 0) {
        perror("tcp_socket_create");
        return 1;
    }

    struct sockaddr_in server_address = {0};
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(SERVER_PORT);

    if (bind(
            server_fd,
            (struct sockaddr *)&server_address,
            sizeof(server_address)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, BACKLOG) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    printf("Server listening on port %d\n", SERVER_PORT);

    uint8_t buffer[256];

    for (;;) {
        struct sockaddr_in client_address = {0};
        socklen_t client_address_len = sizeof(client_address);

        int client_fd = accept(
            server_fd,
            (struct sockaddr *)&client_address,
            &client_address_len);

        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        printf("Client connected\n");

        while (true) {
            tcp_recv_info_t recv_result = 
               tcp_recv(
                    client_fd, 
                    buffer,
                    sizeof(buffer)
                );

            if (recv_result.status == TCP_ERROR) {
                break;
                
            }

            if (recv_result.byte_received > 0) {
                for (size_t i = 0; i < recv_result.byte_received; ++i) {
                    printf("%u ", (unsigned int)buffer[i]);
                }
                printf("\n");
            }

        }

        close(client_fd);
    }

    close(server_fd);
    return 0;
}
