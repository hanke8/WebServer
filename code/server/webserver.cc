#include "webserver.h"
using namespace std;

int SetNonBlocking(int fd) {
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}

WebServer::WebServer(
    int port, int trigMode, int timeoutMS, bool optLinger, int threadNum): 
    port_(port), timeoutMS_(timeoutMS), openLinger_(optLinger), isClosed_(false),
    timer_(new HeapTimer()),threadpool_(new ThreadPool(threadNum)),epoller_(new Epoller()) 
{
    // 获取资源目录
    srcDir_ = getcwd(nullptr, 256);
    assert(srcDir_);
    strncat(srcDir_, "/resources/", 16);
    HttpConnection::userCount = 0;
    HttpConnection::srcDir = srcDir_;

    InitEventMode(trigMode);
    if(!InitListenFd()) {
        isClosed_ = true;
    }
}

WebServer::~WebServer() {
    close(listenFd_);
    free(srcDir_);
    isClosed_ = true;
}

void WebServer::StartService() {
    int timeout = -1;     // -1 无事件时, epollwait将阻塞
    while(!isClosed_) {
        if(timeoutMS_ > 0) {
            timeout = timer_->tick();
        }
        int eventCnt = epoller_->EpollWait(timeout);
        for(int i = 0; i < eventCnt; i++) {
            int fd = epoller_->GetEventFd(i);
            uint32_t event = epoller_->GetEvent(i);
            if(fd == listenFd_) {
                DealListen();
            }
            else if(event & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                assert(users_.count(fd) > 0);
                CloseConn(&users_[fd]);
                users_.erase(fd);
            }
            else if(event & EPOLLIN) {
                assert(users_.count(fd) > 0);
                DealRead(&users_[fd]);
            }
            else if(event & EPOLLOUT) {
                assert(users_.count(fd) > 0);
                DealWrite(&users_[fd]);
            }
            else {
                cout << "Unexpected event" << endl;
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

    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if(listenFd_ < 0) {
        return false;
    }

    // 优雅关闭：当调用close时，不会立即关闭，将剩下的数据发送完毕或超时才关闭
    struct linger optLinger = { 0 };
    if(openLinger_) {
        optLinger.l_onoff = 1;      // 1表示打开，0表示关闭
        optLinger.l_linger = 1;     // 超时时间
    }
    int ret = setsockopt(listenFd_, SOL_SOCKET, SO_LINGER, &optLinger, sizeof(optLinger));
    if(ret < 0) {
        close(listenFd_);
        return false;
    }

    // 设置端口复用：所有fd都可以正常发送数据，只有最后一个fd才可以正常接收数据
    int optval = 1;
    ret = setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));
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

void WebServer::InitEventMode(int trigMode) {
    listenEvent_ = EPOLLRDHUP;
    connEvent_ = EPOLLONESHOT | EPOLLRDHUP;
    switch (trigMode)
    {
    case 0:
        break;
    case 1:
        connEvent_ |= EPOLLET;
        break;
    case 2:
        connEvent_ |= EPOLLET;
        break;
    case 3:
        listenEvent_ |= EPOLLET;
        connEvent_ |= EPOLLET;
        break;
    default:
        listenEvent_ |= EPOLLET;
        connEvent_ |= EPOLLET;
        break;
    }
    // HTTPconnection::isET = (connEvent_ & EPOLLET);
}

void WebServer::CloseConn(HttpConnection* client) {
    assert(client);
    epoller_->DelFd(client->GetFd());
    client->Close();
}

void WebServer::AddClient(int fd, sockaddr_in addr) {
    assert(fd > 0);
    users_[fd].Init(fd, addr);
    if(timeoutMS_>0) {
        timer_->add(fd, timeoutMS_, std::bind(&WebServer::CloseConn, this, &users_[fd]));
    }
    epoller_->AddFd(fd, EPOLLIN | connEvent_);
    SetNonBlocking(fd);
}

void WebServer::DealListen() {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    do {
        int fd = accept(listenFd_, (struct sockaddr*)&addr, &len);
        if(fd <= 0) return;
        if(HttpConnection::userCount >= MAX_FD) {
            sendError(fd, "Server busy!");
            return;
        }
        AddClient(fd, addr);
    } while(listenEvent_ & EPOLLET);
}

void WebServer::sendError(int fd, const char* info)
{
    assert(fd>0);
    send(fd, info, strlen(info), 0);
    close(fd);
}

void WebServer::DealRead(HttpConnection* client) {
    assert(client);
    extentTime(client);
    threadpool_->submit(std::bind(&WebServer::OnRead, this, client));
}

void WebServer::DealWrite(HttpConnection* client) {
    assert(client);
    extentTime(client);
    threadpool_->submit(std::bind(&WebServer::OnWrite, this, client));
}

void WebServer::extentTime(HttpConnection* client) {
    assert(client);
    if(timeoutMS_>0) {
        timer_->reset(client->GetFd(), timeoutMS_);
    }
}

void WebServer::OnRead(HttpConnection* client) {
    assert(client);
    int ret = -1;
    int readErrno = 0;
    ret = client->read(&readErrno);         // 将数据读取到client(httpconn类)的buffer(类成员)里
    if(ret <= 0 && readErrno != EAGAIN) {
        CloseConn(client);
        return;
    }
    OnProcess(client);
}

void WebServer::OnWrite(HttpConnection* client) {
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

void WebServer::OnProcess(HttpConnection* client) {
    if(client->process()) {     // http请求解析
        epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);
    } else {
        epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLIN);
    }
}