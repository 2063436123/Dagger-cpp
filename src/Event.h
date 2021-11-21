//
// Created by Hello Peter on 2021/9/8.
//

#pragma once

#include <functional>
#include <iostream>
#include "ObjectPool.hpp"
#include "Epoller.h"

class Event {
private:
    Event(int fd, Epoller *epoller) : fd_(fd),
                                      events_(0), revents_(0), epoller_(epoller) {
    }
    template<typename T, typename... Args>
    friend T* ObjectPool::getNewObject(Args &&...);
public:

    // 注意：make时已将event添加到epoller的监听列表中
    static Event *make(int fd, Epoller *epoller) {
        Event *event = ObjectPool::getNewObject<Event>(fd, epoller);
        epoller->addEvent(event);
        return event;
    }

    int fd() const { return fd_; }

    Epoller* epoller() {
        return epoller_;
    }

    int events() const { return events_; }

    void setEvents(int events) { events_ = events; }

    int revents() const { return revents_; }

    bool isWritable() const {
        return EPOLLOUT & events_;
    }

    void setRevents(int revents) { revents_ = revents; }

    void setReadable(bool on) {
        int onFlag = EPOLLIN | EPOLLPRI;
        if (on) {
            events_ |= onFlag;
        } else {
            events_ &= ~onFlag;
        }
        epoller_->updateEvent(this);
    }

    void setWritable(bool on) {
        int onFlag = EPOLLOUT;
        if (on) {
            events_ |= onFlag;
        } else {
            events_ &= ~onFlag;
        }
        epoller_->updateEvent(this);
    }

    void setErrorable(bool on) {
        int onFlag = EPOLLERR;
        if (on) {
            events_ |= onFlag;
        } else {
            events_ &= ~onFlag;
        }
        epoller_->updateEvent(this);
    }

    void setReadCallback(std::function<void()> readCallback) {
        readCallback_ = readCallback;
    }

    void setWriteCallback(std::function<void()> writeCallback) {
        writeCallback_ = writeCallback;
    }

    void setErrorCallback(std::function<void()> errorCallback) {
        errorCallback_ = errorCallback;
    }

    void setCloseCallback(std::function<void()> closeCallback) {
        closeCallback_ = closeCallback;
    }

    void handle() {
        // todo events的情况判断是否要再完善？同见setReadable...
        if ((revents_ & EPOLLERR) && errorCallback_) {
            errorCallback_();
            return;
        }
        if ((revents_ & EPOLLIN || revents_ & EPOLLPRI || revents_ & EPOLLRDNORM) && readCallback_) {
            readCallback_();
            return;
        }
        if ((revents_ & EPOLLOUT || revents_ & EPOLLWRNORM) && writeCallback_) {
            writeCallback_();
            return;
        }
        if ((revents_ & EPOLLHUP) && closeCallback_) {
            closeCallback_();
            return;
        }
    }

private:
    int fd_;
    int events_;
    int revents_;
    Epoller *epoller_;
    std::function<void()> readCallback_;
    std::function<void()> writeCallback_;
    std::function<void()> errorCallback_;
    std::function<void()> closeCallback_;
};

