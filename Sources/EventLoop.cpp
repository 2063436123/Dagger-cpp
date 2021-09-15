//
// Created by Hello Peter on 2021/9/12.
//

#include "../EventLoop.h"

thread_local EventLoop *localEventLoop = nullptr;

EventLoop::EventLoop() : looping_(false) {
    if (localEventLoop != nullptr)
        assert(0);
    localEventLoop = this;
    std::cout << "$$$new EventLoop created, in thread " << std::this_thread::get_id() << ", &epoller: " << &epoller_
              << std::endl;
}

