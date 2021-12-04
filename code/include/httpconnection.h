#ifndef HTTP_CONNECTION_H_
#define HTTP_CONNECTION_H_

#include <sys/types.h>
#include <sys/uio.h>     // readv/writev
#include <arpa/inet.h>   // sockaddr_in
#include <stdlib.h>      // atoi()
#include <errno.h> 
#include <atomic>
#include <assert.h>    
#include <unistd.h>

#include "buffer.h"
#include "httprequest.h"
#include "httpresponse.h"

class HttpConnection
{
public:
    HttpConnection();
    ~HttpConnection();
    
    void Init(int sockFd, const sockaddr_in& addr);
    void Close();

    ssize_t read(int* saveErrno);
    ssize_t write(int* saveErrno);
    bool process();

    int GetFd() const;

    int ToWriteBytes();
    bool IsKeepAlive() const;

    static const char* srcDir;
    static bool isEpollET;
    static std::atomic<int> userCount;
    
private:
    int fd_;
    struct sockaddr_in addr_;

    bool isClosed_;

    int iovCnt_;
    struct iovec iov_[2];

    Buffer readBuff_;   // 读缓冲区
    Buffer writeBuff_;  // 写缓冲区

    HttpRequest request_;
    HttpResponse response_;
};

#endif