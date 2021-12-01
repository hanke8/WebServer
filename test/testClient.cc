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
// #include <tool.h>

#define BUF_SIZE 1024

int main(int argc, char* argv[]) {
    if(argc <= 2) {                     // 检查命令行输入是否合法
        printf("usage: %s ip port \n", argv[0]);
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    
    // 创建一个IPv4 socket(目的)地址(具体数据是从用户命令行读入的)
    struct sockaddr_in server_address;
    bzero(&server_address, sizeof(server_address));     // 将一段内存内容全清为零
    server_address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &server_address.sin_addr);   // 将ip转成网络字节序(大端法)
    server_address.sin_port = htons(port);               // 将端口转成从主机字节序(小端法)转成网络字节序
    
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);       //  选择流服务(TCP)
    assert(sockfd >= 0);
    if(connect(sockfd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        printf("connect failed\n");
        close(sockfd);
        return 1;
    }

    // 使用poll(IO复用)同时监听用户的输入和服务器的返回消息
    struct pollfd fds[2];
    fds[0].fd = 0;      // 文件描述符0（标准输入）
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    fds[1].fd = sockfd; // 与服务器通信的socket
    fds[1].events = POLLIN | POLLRDHUP;
    fds[1].revents = 0;
    
    char read_buf[BUF_SIZE];
    int pipefd[2];
    int ret = pipe(pipefd);
    assert(ret != -1);

    while(1) {
        ret = poll(fds, 2, -1);     // timeout设置成-1，使得poll是阻塞的
        if(ret < 0) {
            printf("poll fail\n");
            break;
        }
        
        //处理服务器返回的消息
        if(fds[1].revents & POLLRDHUP) {
            printf("server close the connection\n");
            break;
        }
        else if(fds[1].revents & POLLIN) {
            memset(read_buf, '\0', BUF_SIZE);   // 清空buffer原先的内容
            recv(fds[1].fd, read_buf, BUF_SIZE-1, 0);
            printf("%s\n", read_buf);
        }
        
        // 处理用户的输入
        if(fds[0].revents & POLLIN) {
            // 将标准输入和sockfd直接用管道拼接起来，使得用户输入直接定向发到服务端，从而实现数据零拷贝
            ret = splice(0, NULL, pipefd[1], NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
            ret = splice(pipefd[0], NULL, sockfd, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
        }
    }

    close(sockfd);
    return 0;                                                
}