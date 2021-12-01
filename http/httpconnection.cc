#include "httpconnection.h"
using namespace std;

bool HttpConnection::isEpollET;
std::atomic<int> HttpConnection::userCount;

HttpConnection::HttpConnection() {
    fd_ = -1;
    addr_ = {0};
    isClose_ = true;
}

HttpConnection::~HttpConnection() {
    Close();
}

void HttpConnection::Init(int fd, const sockaddr_in& addr) {
    assert(fd > 0);
    userCount++;
    fd_ = fd;
    addr_ = addr;
    // todo: clear write、read buffer
    isClose_ = false;
    // todo: log
}

void HttpConnection::Close() {
    // todo: 解除内存映射
    if(isClose_ == false) {     // 防止不同线程重复关闭
        isClose_ = true;
        userCount--;
        close(fd_);
        // todo: log
    }
}

ssize_t HttpConnection::read(int* saveErrno) {
    ssize_t len = -1;
    do {
        // len = readBuff_.ReadFd(fd_, saveErrno);
        if (len <= 0) {
            break;
        }
    } while(isEpollET);
    return len;
}

ssize_t HttpConnection::write(int* saveErrno) {
    ssize_t len = -1;
    // todo
    return len;
}

bool HttpConnection::process() {
    // todo
    return true;
}

int HttpConnection::GetFd() const {
    return fd_;
};