#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

#include "network/tcp.h"

static void ZeroLengthRequest(void) 
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(sockfd >= 0);

    tcp_send_info_t result = 
        tcp_send_all(sockfd, NULL, 0);
    
    assert(result.status == TCP_SUCCESS);
    assert(result.status == 0);

    close(sockfd);
}

static void InvalidSocketDescriptor(void) 
{
    uint8_t buffer = {0};

    tcp_send_info_t result =
        tcp_send_all(-1, &buffer, sizeof(buffer));

    assert(result.status == TCP_ERROR);
    assert(result.bytes_sent == 0);
}

static void SendToClosedPeer(void)
{
    int sockets[2];
    int socksfd = socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);

    assert(socksfd == 0);

    close(sockets[0]);

    uint8_t buffer = {0};

    tcp_send_info_t result = 
        tcp_send_all(socksfd, &buffer, sizeof(buffer));

    assert(result.status == TCP_ERROR);
    assert(result.bytes_sent == 0);

    close(sockets[1]);
}

static void SendAllData(void)
{
    int sockets[2];
    int socksfd = socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);

    assert(socksfd == 0);

    uint8_t buffer[] = "hello";

    tcp_send_info_t result =
        tcp_send_all(sockets[0], buffer, sizeof(buffer));


    uint8_t recv_buffer[sizeof(buffer)];
    ssize_t bytes_received = recv(sockets[1], recv_buffer, sizeof(recv_buffer), 0);

        
    assert(result.status == TCP_SUCCESS);
    assert(result.bytes_sent == sizeof(buffer));

    assert(bytes_received == sizeof(buffer));
    assert(memcmp(buffer, recv_buffer, sizeof(buffer)) == 0);

    close(sockets[0]);
    close(sockets[1]);
}

static void SocketCreateSuccess(void)
{
    int result = tcp_socket_create();

    assert(result >= 0);

    close(result);
}

static void TcpConnect(void) 
{
    int sockfd = tcp_socket_create();

    assert(sockfd >= 0);
    if (sockfd < 0) {
        return;
    }

    struct sockaddr_in server_address = {0};
    socklen_t address_len = sizeof(server_address);
    
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    server_address.sin_port = 0;

    assert(bind(
        sockfd, 
        (const struct sockaddr *)&server_address, 
        address_len
    ) == 0);

    assert(listen(sockfd, 5) == 0);

    assert(getsockname(
        sockfd, 
        (struct sockaddr *)&server_address, 
        &address_len
    ) == 0);

    int cliendfd = tcp_socket_create();
    assert(cliendfd >= 0);

    int connect_result = tcp_connect(cliendfd, &server_address);

    if(connect_result == -1) {
        perror("connect");
        close(sockfd);
        close(cliendfd);
        return;
    }

    assert(connect_result == 0);

    int accepted_fd = accept(sockfd, NULL, NULL);
 
    if (accepted_fd == -1) {
        perror("accepted");
        close(sockfd);
        close(cliendfd);
        return;
    }

    assert(accepted_fd >= 0);

    close(sockfd);
    close(cliendfd);
    close(accepted_fd);
}


int main(void) {
    ZeroLengthRequest();
    InvalidSocketDescriptor();
    SendToClosedPeer();
    SendAllData();
    SocketCreateSuccess();
    TcpConnect();

    return 0;
}   
