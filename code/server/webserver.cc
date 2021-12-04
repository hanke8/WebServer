#include "webserver.h"
using namespace std;

int SetNonBlocking(int fd) {
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}

WebServer::WebServer(int port): port_(port), isClosed_(false), epoller_(new Epoller()) {
    // todo：获取资源目录、http、sql
    srcDir_ = getcwd(nullptr, 256);
    assert(srcDir_);
    strncat(srcDir_, "/resources/", 16);
    HttpConnection::userCount = 0;
    HttpConnection::srcDir = srcDir_;


    listenEvent_ = EPOLLRDHUP | EPOLLET;  // todo: wait to change
    if(!InitListenFd()) {
        isClosed_ = true;
    }
    connEvent_ = EPOLLONESHOT | EPOLLRDHUP | EPOLLET;
}

// 疑惑：析构函数会被调用吗？
WebServer::~WebServer() {
    close(listenFd_);
    free(srcDir_);
    isClosed_ = true;
    // todo
}

void WebServer::StartService() {
    int timeoutMs = -1;     // 无事件时, epollwait将阻塞
    while(!isClosed_) {
        // todo: 更新超时时间
        int ret = epoller_->EpollWait(timeoutMs);
        for(int i = 0; i < ret; i++) {  // 处理事件
            int fd = epoller_->GetEventFd(i);
            uint32_t event = epoller_->GetEvent(i);
            if(fd == listenFd_) {
                DealListen();
                cout << "test：连接事件" << endl;
            }
            else if(event & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                assert(users_.count(fd) > 0);
                CloseConn(&users_[fd]);
                cout << "test：关闭事件" << endl;
                // users_.erase(fd);
            }
            else if(event & EPOLLIN) {
                assert(users_.count(fd) > 0);
                DealRead(&users_[fd]);
                cout << "test：可读事件" << endl;
            }
            else if(event & EPOLLOUT) {
                assert(users_.count(fd) > 0);
                DealWrite(&users_[fd]);
                cout << "test：可写事件" << endl;
            }
            else {
                // todo: log
            }
        }
    }
}

bool WebServer::InitListenFd() {
    if(port_ > 65535 || port_ < 1024) {
        return false;
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port_);
    // todo: 优雅关闭

    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if(listenFd_ < 0) {
        return false;
    }
    // 设置端口复用
    int optval = 1;
    int ret = setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));
    if(ret == -1) {
        close(listenFd_);
        return false;
    }
    ret = bind(listenFd_, (struct sockaddr*)&addr, sizeof(addr));
    if(ret < 0) {
        close(listenFd_);
        return false;
    }
    ret = listen(listenFd_, 6);
    if(ret < 0) {
        close(listenFd_);
        return false;
    }

    ret = epoller_->AddFd(listenFd_, listenEvent_ | EPOLLIN);
    if(ret == false) {
        close(listenFd_);
        return false;
    }
    SetNonBlocking(listenFd_);
    return true;
}

void WebServer::CloseConn(HttpConnection* client) {
    assert(client);
    // todo: log
    epoller_->DelFd(client->GetFd());
    client->Close();
}

void WebServer::AddClient(int fd, sockaddr_in addr) {
    assert(fd > 0);
    users_[fd].Init(fd, addr);
    epoller_->AddFd(fd, EPOLLIN | connEvent_);
    SetNonBlocking(fd);
    // todo: log
}

void WebServer::DealListen() {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    do {
        int fd = accept(listenFd_, (struct sockaddr*)&addr, &len);
        if(fd <= 0) return;
        if(HttpConnection::userCount >= MAX_FD) {
            // todo
            return;
        }
        AddClient(fd, addr);
    } while(listenEvent_ & EPOLLET);
}

void WebServer::DealRead(HttpConnection* client) {
    assert(client);
    // todo
    int ret = -1;
    int readErrno = 0;
    ret = client->read(&readErrno);         // 将数据读取到client(httpconn类)的buffer(类成员)里
    if(ret <= 0 && readErrno != EAGAIN) {
        CloseConn(client);
        return;
    }
    OnProcess(client);
}

void WebServer::OnProcess(HttpConnection* client) {
    if(client->process()) {     // http请求解析
        epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);
    } else {
        epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLIN);
    }
}

void WebServer::DealWrite(HttpConnection* client) {
    assert(client);
    int ret = -1;
    int writeErrno = 0;
    ret = client->write(&writeErrno);
    if(client->ToWriteBytes() == 0) {
        /* 传输完成 */
        if(client->IsKeepAlive()) {
            OnProcess(client);
            return;
        }
    }
    else if(ret < 0) {
        if(writeErrno == EAGAIN) {
            /* 继续传输 */
            epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);
            return;
        }
    }
    CloseConn(client);
}