//
// Created by Hello Peter on 2021/9/8.
//

#include "../Epoller.h"
#include "../Event.h"
#include <iostream>
#include <unistd.h>
#include <cstring>

using namespace std;

// todo 怎么给eventList加锁，全局一个锁不行
std::mutex evMutex_;

// 类外初始化
thread_local epoll_event Epoller::evlist[MAX_EVENTS];

Epoller::Epoller() {
    // fixme 不能每个线程都调用一次epoll_create吗？
    epollfd_ = epoll_create1(0);
    if (epollfd_ < 0)
        assert(0);
}

Epoller::~Epoller() = default;

void Epoller::addEvent(std::shared_ptr<Event> event) {
    struct epoll_event ev{.events = static_cast<uint32_t>(event->events()),
            .data = epoll_data_t{.fd = event->fd()}};
    int ret = epoll_ctl(epollfd_, EPOLL_CTL_ADD, event->fd(), &ev);
    if (ret < 0)
        assert(0);

    unique_lock<mutex> ul(evMutex_);
    if (eventList_.insert({event->fd(), event}).second == false)
        assert(0);
}

std::shared_ptr<Event> Epoller::getEvent(int fd) {
    unique_lock<mutex> ul(evMutex_);
    auto iter = eventList_.find(fd);
    if (iter == eventList_.end())
        return {};
    return iter->second;
}

void Epoller::updateEvent(int fd) {
    auto event = getEvent(fd);
    if (!event) {
        assert(0); // not exist
    }
    struct epoll_event ev{.events = static_cast<uint32_t>(event->events()),
            .data = epoll_data_t{.fd = event->fd()}};
    int ret = epoll_ctl(epollfd_, EPOLL_CTL_MOD, event->fd(), &ev);
    if (ret < 0)
        assert(0);
}

void Epoller::removeEvent(int fd) {
    auto event = getEvent(fd);
    if (!event) {
        assert(0); // not exist
    }
    int ret = epoll_ctl(epollfd_, EPOLL_CTL_DEL, event->fd(), nullptr);
    if (ret < 0)
        assert(0);
    unique_lock<mutex> ul(evMutex_);
    eventList_.erase(fd);
}

std::vector<std::shared_ptr<Event>> Epoller::poll(int timeout) {
    std::vector<std::shared_ptr<Event>> res;
    con:
    int ret = epoll_wait(epollfd_, evlist, MAX_EVENTS, timeout);
    if (ret < 0) {
        cout << strerror(errno) << endl;
        if (errno == EINTR)
            goto con;
        assert(0);
    }

    // fixed 之前addEvent()中插入eventList中的Event消失了
    // fixed: 为什么eventList是空的？ 因为loops_数组移动位置了

    unique_lock<mutex> ul(evMutex_);
    for (int i = 0; i < ret; i++) {
        auto iter = eventList_.find(evlist[i].data.fd);
        assert(iter != eventList_.end());
        iter->second->setRevents(evlist[i].events);
        res.push_back(iter->second);
    }
    ul.unlock();
    return res;
}

//void handleRead(int fd) {
//    char buf[2000];
//    int n = read(fd, buf, 2000);
//    cout << "\treadN: " << n << "(" << std::string(buf, n) << ")" << endl;
//}

//int main() {
//    int fd = STDIN_FILENO;
//    Epoller epoller;
//    auto event = make_shared<Event>(fd, &epoller);
//    epoller.addEvent(event);
//
//    event->setReadable(true);
//    auto readCallback = [fd = event->fd()]() {
//        handleRead(fd);
//    };
//    event->setReadCallback(readCallback);
//
//    auto res = epoller.poll();
//    cout << "size: " << res.size() << endl;
//    for (auto event : res) {
//        cout << "fd: " << event->fd() << ", revents: " << event->revents() << endl;
//        event->handle();
//    }
//}