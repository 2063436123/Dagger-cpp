//
// Created by Hello Peter on 2021/9/8.
//

#include "../Epoller.h"
#include "../Event.h"
#include <iostream>
#include <unistd.h>
#include <cstring>

using namespace std;

// 类外初始化
thread_local epoll_event Epoller::evlist[MAX_EVENTS];

Epoller::Epoller() {
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
    eventList.insert({event->fd(), event});
}

std::shared_ptr<Event> Epoller::getEvent(int fd) {
    auto iter = eventList.find(fd);
    if (iter == eventList.end())
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
    eventList.erase(fd);
}

std::vector<std::shared_ptr<Event>> Epoller::poll(int timeout) {
    std::vector<std::shared_ptr<Event>> res;
    int ret = epoll_wait(epollfd_, evlist, MAX_EVENTS, timeout);
    if (ret < 0) {
        cout << strerror(errno) << endl;
        assert(0);
    }
    {
        for (auto& x : evlist) {
            cout << "epoll_wait ret: " << x.data.fd << endl;
        }
    }
    for (int i = 0; i < ret; i++) {
        auto iter = eventList.find(evlist[i].data.fd);
        assert(iter != eventList.end());
        iter->second->setRevents(evlist[i].events);
        res.push_back(iter->second);
    }
    return res;
}

void handleRead(int fd) {
    char buf[2000];
    int n = read(fd, buf, 2000);
    cout << "\treadN: " << n << "(" << std::string(buf, n) << ")" << endl;
}

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