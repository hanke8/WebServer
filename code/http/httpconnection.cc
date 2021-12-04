#include "httpconnection.h"
using namespace std;

const char* HttpConnection::srcDir;
bool HttpConnection::isEpollET;
std::atomic<int> HttpConnection::userCount;

HttpConnection::HttpConnection() {
    fd_ = -1;
    addr_ = {0};
    isClosed_ = true;
}

HttpConnection::~HttpConnection() {
    Close();
}

void HttpConnection::Init(int fd, const sockaddr_in& addr) {
    assert(fd > 0);
    userCount++;
    fd_ = fd;
    addr_ = addr;
    readBuff_.RetrieveAll();
    writeBuff_.RetrieveAll();
    isClosed_ = false;
    // todo: log
}

void HttpConnection::Close() {
    // todo: 解除内存映射
    if(isClosed_ == false) {     // 防止不同线程重复关闭
        isClosed_ = true;
        userCount--;
        close(fd_);
        // todo: log
    }
}

ssize_t HttpConnection::read(int* saveErrno) {
    ssize_t len = -1;
    do {
        len = readBuff_.ReadFd(fd_, saveErrno);
        if (len <= 0) {
            break;
        }
    } while(isEpollET);
    return len;
}

ssize_t HttpConnection::write(int* saveErrno) {
    ssize_t len = -1;
    do {
        len = writev(fd_, iov_, iovCnt_);
        if(len <= 0) {
            *saveErrno = errno;
            break;
        }
        if(iov_[0].iov_len + iov_[1].iov_len  == 0) {    // 传输完成
            break; 
        }
        else if(static_cast<size_t>(len) > iov_[0].iov_len) {    // iov_[0] 传输完成
            iov_[1].iov_base = (uint8_t*) iov_[1].iov_base + (len - iov_[0].iov_len);
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            if(iov_[0].iov_len) {
                writeBuff_.RetrieveAll();
                iov_[0].iov_len = 0;
            }
        }
        else {                  // iov_[0]还在传输中
            iov_[0].iov_base = (uint8_t*)iov_[0].iov_base + len; 
            iov_[0].iov_len -= len; 
            writeBuff_.Retrieve(len);
        }
    } while(isEpollET || ToWriteBytes() > 10240);
    return len;
}

bool HttpConnection::process() {
    request_.Init();
    if(readBuff_.WritableBytes() <= 0) {
        return false;
    }
    else if(request_.parse(readBuff_)) {
        // log
        response_.Init(srcDir, request_.path(), request_.IsKeepAlive(), 200);
    }
    else {
        response_.Init(srcDir, request_.path(), false, 400);
    }
    
    response_.MakeResponse(writeBuff_);
    // 响应头
    iov_[0].iov_base = const_cast<char*>(writeBuff_.GetWritePos());
    iov_[0].iov_len = writeBuff_.WritableBytes();
    iovCnt_ = 1;

    // 文件
    if(response_.FileLen() > 0 && response_.File()) {
        iov_[1].iov_base = response_.File();
        iov_[1].iov_len = response_.FileLen();
        iovCnt_ = 2;
    }
    // log
    return true;
}

int HttpConnection::GetFd() const {
    return fd_;
};

int HttpConnection::ToWriteBytes() {
    return iov_[0].iov_len + iov_[1].iov_len;
}

bool HttpConnection::IsKeepAlive() const {
    return request_.IsKeepAlive();
}