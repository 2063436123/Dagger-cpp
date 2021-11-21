//
// Created by Hello Peter on 2021/9/8.
//

#include "../Epoller.h"
#include "../Event.h"
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <array>
#include "../SpinLock.h"

using namespace std;

using Mutex = std::mutex;
// note 减少锁争用，hash(epollerId)选择一个锁
std::array<Mutex, 5> g_mutexs;

// 类外初始化
thread_local epoll_event Epoller::evlist[MAX_EVENTS];

static Mutex &getLock(int id) {
    return g_mutexs[(size_t) id % g_mutexs.size()];
}

Epoller::Epoller() {
    epollfd_ = epoll_create1(0);
    if (epollfd_ < 0)
        Logger::sys("epoll_create error");
}

Epoller::~Epoller() = default;
// 即使一个Epoller只属于一个EventLoop，Epoller操作需要加锁，因为主线程和工作线程会有竞态条件

void Epoller::addEvent(Event *event) {
    struct epoll_event ev{.events = static_cast<uint32_t>(event->events()),
//            .data = epoll_data_t{.fd = event->fd()}};
            .data = epoll_data_t{.ptr = event}};
    int ret = epoll_ctl(epollfd_, EPOLL_CTL_ADD, event->fd(), &ev);
    if (ret < 0)
        Logger::sys("in addEvent, epoll_ctl error");
}

void Epoller::updateEvent(Event *event) {
    if (!event) {
        Logger::fatal("in updateEvent, Event is nullptr\n"); // not exist
    }
    struct epoll_event ev{.events = static_cast<uint32_t>(event->events()),
//            .data = epoll_data_t{.fd = event->fd()}};
            .data = epoll_data_t{.ptr = event}};
    int ret = epoll_ctl(epollfd_, EPOLL_CTL_MOD, event->fd(), &ev);
    if (ret < 0)
        Logger::sys("in updateEvent, epoll_ctl error. {}", event->fd());
}

void Epoller::removeEvent(Event *event) {
    if (!event) {
        Logger::fatal("in removeEvent, Event is nullptr"); // not exist
    }
    int ret = epoll_ctl(epollfd_, EPOLL_CTL_DEL, event->fd(), nullptr);
    if (ret < 0)
        Logger::sys("epoll_ctl error");
}

std::vector<Event*> Epoller::poll(int timeout) {
    std::vector<Event*> res;
    int ret = epoll_wait(epollfd_, evlist, MAX_EVENTS, timeout);
    if (ret < 0 && errno != EINTR) {
        Logger::sys("epoll_wait error");
        return {};
    }

    for (int i = 0; i < ret; i++) {
        Event* event = static_cast<Event *>(evlist[i].data.ptr);
        event->setRevents(evlist[i].events);
        res.push_back(event);
    }
    return res;
}
