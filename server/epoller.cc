#include "epoller.h"

Epoller::Epoller(int max_event):epollFd_(epoll_create(512)), events_(max_event) {
    assert(epollFd_ >= 0 && events_.size() > 0);
}

Epoller::~Epoller() {
    close(epollFd_);
}

bool Epoller::AddFd(int fd, uint32_t events) {
    if(fd < 0) return false;
    epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;
    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ev);
}

bool Epoller::ModFd(int fd, uint32_t events) {
    if(fd < 0) return false;
    epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;
    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &ev);
}

bool Epoller::DelFd(int fd) {
    if(fd < 0) return false;
    epoll_event ev;
    ev.data.fd = fd;
    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, &ev);
}

int Epoller::EpollWait(int timeoutMs) {
    return epoll_wait(epollFd_, &events_[0], static_cast<int>(events_.size()), timeoutMs);
}

int Epoller::GetEventFd(size_t i) const {
    assert(i >= 0 && i < events_.size());
    return events_[i].data.fd;
}

uint32_t Epoller::GetEvent(size_t i) const {
    assert(i >= 0 && i < events_.size());
    return events_[i].events;
}
