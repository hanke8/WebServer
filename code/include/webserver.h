#ifndef WEBSERVER_H_
#define WEBSERVER_H_

#include <memory>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>

#include "epoller.h"
#include "httpconnection.h"

class WebServer {
public:
    WebServer(int port);
    ~WebServer();

    void StartService();   // 启动服务
    
private:
    bool InitListenFd();
    void AddClient(int fd, sockaddr_in addr);
    void CloseConn(HttpConnection* client);

    void DealListen();
    void DealWrite(HttpConnection* client);
    void DealRead(HttpConnection* client);

    void OnProcess(HttpConnection* client);

    int port_;       // 端口号
    int listenFd_;   // 监听socket
    static const int MAX_FD = 65535;
    char* srcDir_;
    bool isClosed_;

    uint32_t listenEvent_;
    uint32_t connEvent_;

    std::unique_ptr<Epoller> epoller_;
    std::unordered_map<int, HttpConnection> users_;
};

#endif