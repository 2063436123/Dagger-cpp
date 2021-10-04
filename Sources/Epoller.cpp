//
// Created by Hello Peter on 2021/9/8.
//

#include "../Epoller.h"
#include "../Event.h"
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <array>

using namespace std;

// note 怎么给eventList加锁，hash(epollerId)选择一个锁
std::array<std::mutex, 5> g_mutexs;

// 类外初始化
thread_local epoll_event Epoller::evlist[MAX_EVENTS];

// note: 这里使用hash(epollerId)来选择使用的锁
static std::mutex& getLock(int id) {
    return g_mutexs[(size_t)id % g_mutexs.size()];
}

Epoller::Epoller() {
    // fixme 不能每个线程都调用一次epoll_create吗？
    epollfd_ = epoll_create1(0);
    cout << "epollfd: " << epollfd_ << endl;
    if (epollfd_ < 0)
        Logger::sys("epoll_create error");
}

Epoller::~Epoller() = default;

void Epoller::addEvent(std::shared_ptr<Event> event) {
    struct epoll_event ev{.events = static_cast<uint32_t>(event->events()),
            .data = epoll_data_t{.fd = event->fd()}};
    int ret = epoll_ctl(epollfd_, EPOLL_CTL_ADD, event->fd(), &ev);
    if (ret < 0)
        Logger::sys("epoll_ctl error");

    unique_lock<mutex> ul(getLock(epollId()));
    if (eventList_.insert({event->fd(), event}).second == false)
        Logger::fatal("insert error");
}

std::shared_ptr<Event> Epoller::getEvent(int fd) {
    unique_lock<mutex> ul(getLock(epollId()));
    auto iter = eventList_.find(fd);
    if (iter == eventList_.end())
        return {};
    return iter->second;
}

void Epoller::updateEvent(int fd) {
    auto event = getEvent(fd);
    if (!event) {
        Logger::fatal("in updateEvent can't getEvent"); // not exist
    }
    struct epoll_event ev{.events = static_cast<uint32_t>(event->events()),
            .data = epoll_data_t{.fd = event->fd()}};
    int ret = epoll_ctl(epollfd_, EPOLL_CTL_MOD, event->fd(), &ev);
    if (ret < 0)
        Logger::sys("epoll_ctl error.");
}

void Epoller::removeEvent(int fd) {
    auto event = getEvent(fd);
    if (!event) {
        Logger::fatal("in removeEvent, getEvent error"); // not exist
    }
    int ret = epoll_ctl(epollfd_, EPOLL_CTL_DEL, event->fd(), nullptr);
    if (ret < 0)
        Logger::sys("epoll_ctl error");
    unique_lock<mutex> ul(getLock(epollId()));
    eventList_.erase(fd);
}

std::vector<std::shared_ptr<Event>> Epoller::poll(int timeout) {
    std::vector<std::shared_ptr<Event>> res;
    con:
    int ret = epoll_wait(epollfd_, evlist, MAX_EVENTS, timeout);
    if (ret < 0) {
        Logger::sys("epoll_wait error");
    }

    // fixed 之前addEvent()中插入eventList中的Event消失了
    // fixed: 为什么eventList是空的？ 因为loops_数组移动位置了

    unique_lock<mutex> ul(getLock(epollId()));
    for (int i = 0; i < ret; i++) {
        auto iter = eventList_.find(evlist[i].data.fd);
        assert(iter != eventList_.end());
        iter->second->setRevents(evlist[i].events);
        res.push_back(iter->second);
    }
    ul.unlock();
    return res;
}
