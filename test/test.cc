#define _GNU_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

int main() {
    const char* ip = "127.0.0.1";

    struct sockaddr_in server_address;
    bzero(&server_address, sizeof(server_address));     // 将一段内存内容全清为零
    server_address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &server_address.sin_addr);   // 将ip转成网络字节序(大端法)
    server_address.sin_port = htons(12345);               // 将端口转成从主机字节序(小端法)转成网络字节序
    
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);       //  选择流服务(TCP)
    
    int ret = close(sockfd);
    printf("test: %d ", ret);

    ret = close(sockfd);
    printf("test: %d ", ret);
    return 0;
}