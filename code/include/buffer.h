#ifndef BUFFER_H_
#define BUFFER_H_

#include <cstring>   //perror
#include <iostream>
#include <unistd.h>  // write
#include <sys/uio.h> //readv
#include <vector> //readv
#include <atomic>
#include <assert.h>

class Buffer {
public:
    Buffer(int initBuffSize = 1024);
    ~Buffer() = default;

    ssize_t ReadFd(int fd, int* saveErrno);
    ssize_t WriteFd(int fd, int* saveErrno);    // 未使用
    void Retrieve(size_t len);                  // 从buffer_里面取出数据
    void RetrieveUntil(const char* end);
    void RetrieveAll() ;
    std::string RetrieveAllToStr();
    
    size_t ReadableBytes() const;
    size_t WritableBytes() const;
    size_t PrependableBytes() const;

    char* GetReadPos();
    const char* GetReadPosConst() const;
    const char* GetWritePos() const;

    void EnsureReadable(size_t len);
    void HasRead(size_t len);

    void Append(const std::string& str);
    void Append(const char* str, size_t len);
    void Append(const void* data, size_t len);
    void Append(const Buffer& buff);
    
private:
    char* GetBasePtr();
    const char* GetBasePtr() const;
    void MakeSpace_(size_t len);

    std::vector<char> buffer_;
    std::atomic<std::size_t> readPos_;
    std::atomic<std::size_t> writePos_;
};

#endif