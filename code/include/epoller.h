#ifndef EPOLLER_H_
#define EPOLLER_H_

#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <vector>
#include <errno.h>

class Epoller {
public:
    explicit Epoller(int max_event = 1024);
    ~Epoller();

    bool AddFd(int fd, uint32_t events);
    bool ModFd(int fd, uint32_t events);
    bool DelFd(int fd);

    int EpollWait(int timeout_ms = -1);

    int GetEventFd(size_t i) const;
    uint32_t GetEvent(size_t i) const;

private:
    int epollFd_;   // epoll_create得到的fd
    std::vector<struct epoll_event> events_;
};

#endif