#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "network/tcp.h"

//send
tcp_send_info_t tcp_send_all(
    int sockfd,
    const uint8_t* buffer,
    size_t size
) {
    tcp_send_info_t result = {
        .status = TCP_ERROR,
        .bytes_sent = 0
    };

    if (sockfd < 0 || (buffer == NULL && size != 0)) {
        return result;
    }

    if (size == 0) {
        result.status = TCP_SUCCESS;
        return result;
    }
    
    size_t remaining = size;
    ssize_t bytes_sent = 0;

    while (remaining > 0) {

        bytes_sent = 
            send(
                sockfd, 
                buffer + result.bytes_sent, 
                remaining, 
                MSG_NOSIGNAL
            );

        if (bytes_sent == -1) {
            if (errno == EINTR) {
                continue;
            }
            return result;
        } else if (bytes_sent == 0) {
            return result;
        }

        remaining -= (size_t)bytes_sent;
        result.bytes_sent += (size_t)bytes_sent;
    }

    result.status = TCP_SUCCESS;
    return result;
}

int tcp_socket_create(void)
{
    return socket(AF_INET, SOCK_STREAM, 0);
}

int tcp_connect(int sockfd, const struct sockaddr_in* address)
{
    return connect(sockfd, (const struct sockaddr *)address, sizeof(*address));
}

//recv
tcp_recv_info_t tcp_recv(
    int sockfd,
    uint8_t *buffer,
    size_t capacity
)
{
    tcp_recv_info_t result = {
        .status = TCP_ERROR,
        .byte_received = 0
    };

    if (sockfd == -1 || (buffer == NULL && capacity != 0)) {
        return result;
    }

    if (capacity == 0) {
        result.status = TCP_SUCCESS;
        return result;
    }

    ssize_t bytes_received;

    do {
        bytes_received = recv(sockfd, buffer, capacity, 0);
    } while (bytes_received == -1 && errno == EINTR);

    if (bytes_received <= 0) {
        return result;
    }

    result.status = TCP_SUCCESS;    
    result.byte_received = (size_t)bytes_received;
    return result;
}
