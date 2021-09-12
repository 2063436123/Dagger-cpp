//
// Created by Hello Peter on 2021/9/12.
//

#include "EventLoop.h"

thread_local EventLoop *localEventLoop = nullptr;

EventLoop::EventLoop() : looping_(false) {
    if (localEventLoop != nullptr)
        assert(0);
    localEventLoop = this;
}

