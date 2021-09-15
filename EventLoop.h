//
// Created by Hello Peter on 2021/9/12.
//

#ifndef TESTLINUX_EVENTLOOP_H
#define TESTLINUX_EVENTLOOP_H

#include <thread>
#include <mutex>
#include <iostream>
#include "Event.h"

class EventLoop;

class EventLoop {
public:
    EventLoop();

    // can be called by TcpServer etc.
    void init() {
        looping_ = true;
    }

    void start() {
        while (looping_) {
            auto ret = epoller_.poll();
            for (auto &event : ret)
                event->handle();
        }
    }


    Epoller *epoller() {
        return &epoller_;
    }

    void stop() {
        looping_ = false;
        // do sth...
    }

private:
    Epoller epoller_; // 生命期要长于所有的Event
    bool looping_;
};


#endif //TESTLINUX_EVENTLOOP_H
