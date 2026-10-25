#ifndef TCP_H
#define TCP_H

#include <stdint.h>
#include <stddef.h>
#include <netinet/in.h>

typedef enum {
    TCP_SUCCESS,
    TCP_DISCONNECTED,
    TCP_ERROR
} tcp_result_t; 

typedef struct {
    tcp_result_t status;
    size_t bytes_sent;
} tcp_send_info_t;

tcp_send_info_t tcp_send_all(
    int sockfd, 
    const uint8_t* buffer, 
    size_t size
);

int tcp_socket_create(void);

int tcp_connect(int sockfd, const struct sockaddr_in* address);


typedef struct {
    tcp_result_t status;
    size_t byte_received;
} tcp_recv_info_t;

tcp_recv_info_t tcp_recv(
    int sockfd,
    uint8_t *buffer,
    size_t capacity
);


#endif
