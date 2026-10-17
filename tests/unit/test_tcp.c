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


int main(void) {
    ZeroLengthRequest();
    InvalidSocketDescriptor();
    SendToClosedPeer();
    SendAllData();
    SocketCreateSuccess();


    return 0;
}   
