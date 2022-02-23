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
#include "timer.h"
#include "threadpool.h"
#include "httpconnection.h"

class WebServer {
public:
    WebServer(int port, int trigMode, int timeoutMS, bool optLinger,int threadNum);
    ~WebServer();

    void StartService();   // 启动服务
    
private:
    bool InitListenFd();
    void InitEventMode(int trigMode);

    void AddClient(int fd, sockaddr_in addr);
    void CloseConn(HttpConnection* client);

    void DealListen();
    void DealWrite(HttpConnection* client);
    void DealRead(HttpConnection* client);
    void OnRead(HttpConnection* client);
    void OnWrite(HttpConnection* client);
    void OnProcess(HttpConnection* client);

    void sendError(int fd, const char* info);
    void extentTime(HttpConnection* client);

    static const int MAX_FD = 65535;

    int port_;
    int timeoutMS_;
    int listenFd_;
    bool openLinger_;
    char* srcDir_;
    bool isClosed_;

    uint32_t listenEvent_;
    uint32_t connEvent_;

    std::unique_ptr<HeapTimer> timer_;
    std::unique_ptr<Epoller> epoller_;
    std::unique_ptr<ThreadPool> threadpool_;
    std::unordered_map<int, HttpConnection> users_;
};

#endif