#include "buffer.h"

Buffer::Buffer(int initBuffSize): buffer_(initBuffSize), readPos_(0), writePos_(0) {}

ssize_t Buffer::ReadFd(int fd, int* saveErrno) {
    char extraBuff[65535];
    const size_t readSize = ReadableBytes();
    // 分散读，保证数据全部读完
    struct iovec iov[2];
    iov[0].iov_base = GetBasePtr() + readPos_;
    iov[0].iov_len = readSize;
    iov[1].iov_base = extraBuff;
    iov[1].iov_len = sizeof(extraBuff);

    const ssize_t len = readv(fd, iov, 2);
    if(len < 0) {
        *saveErrno = errno;
    }
    else if(static_cast<size_t>(len) <= readSize) {
        readPos_ += len;
    }
    else {
        readPos_ = buffer_.size();
        Append(extraBuff, len - readSize);
    }
    return len;
}

ssize_t Buffer::WriteFd(int fd, int* saveErrno) {
    size_t writeSize =  WritableBytes();
    ssize_t len = write(fd, GetWritePos(), writeSize);
    if(len < 0) {
        *saveErrno = errno;
        return len;
    } 
    writePos_ += len;
    return len;
}

void Buffer::Retrieve(size_t len) {
    assert(len <= WritableBytes());
    writePos_ += len;
}

void Buffer::RetrieveUntil(const char* end) {
    assert(GetWritePos() <= end );
    Retrieve(end - GetWritePos());
}

void Buffer::RetrieveAll() {
    bzero(&buffer_[0], buffer_.size());
    readPos_ = 0;
    writePos_ = 0;
}

std::string Buffer::RetrieveAllToStr() {
    std::string str(GetWritePos(), WritableBytes());
    RetrieveAll();
    return str;
}

size_t Buffer::ReadableBytes() const {
    return buffer_.size() - readPos_;
}

size_t Buffer::WritableBytes() const {
    return readPos_ - writePos_;
}

size_t Buffer::PrependableBytes() const {
    return writePos_;
}

char* Buffer::GetReadPos() {
    return GetBasePtr() + readPos_;
}

const char* Buffer::GetReadPosConst() const {
    return GetBasePtr() + readPos_;
}

const char* Buffer::GetWritePos() const {
    return GetBasePtr() + writePos_;
}

void Buffer::EnsureReadable(size_t len) {
    if(ReadableBytes() < len) {
        MakeSpace_(len);
    }
    assert(ReadableBytes() >= len);
}

void Buffer::HasRead(size_t len) {
    readPos_ += len;
} 

void Buffer::Append(const std::string& str) {
    Append(str.data(), str.length());
}

void Buffer::Append(const void* data, size_t len) {
    assert(data);
    Append(static_cast<const char*>(data), len);
}

void Buffer::Append(const char* str, size_t len) {
    assert(str);
    EnsureReadable(len);
    std::copy(str, str + len, GetReadPos());
    HasRead(len);
}

void Buffer::Append(const Buffer& buff) {
    Append(buff.GetWritePos(), buff.WritableBytes());
}

char* Buffer::GetBasePtr() {
    return &*buffer_.begin();
}

const char* Buffer::GetBasePtr() const {
    return &*buffer_.begin();
}

void Buffer::MakeSpace_(size_t len) {
    if(ReadableBytes() + PrependableBytes() < len) {
        buffer_.resize(readPos_ + len + 1);
    } 
    else {
        size_t writeSize = WritableBytes();
        std::copy(GetBasePtr() + writePos_, GetBasePtr() + readPos_, GetBasePtr());
        writePos_ = 0;
        readPos_ = writePos_ + writeSize;
        assert(writeSize == WritableBytes());
    }
}