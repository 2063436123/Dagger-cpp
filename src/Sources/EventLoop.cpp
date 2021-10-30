//
// Created by Hello Peter on 2021/9/12.
//

#include "../EventLoop.h"
#include <sys/eventfd.h>

thread_local EventLoop *localEventLoop = nullptr;

EventLoop::EventLoop() : looping_(false), isInTaskHanding_(false) {

}

void EventLoop::init() {
    if (localEventLoop != nullptr)
        Logger::fatal("localEventLoop isn't nullptr!");
    localEventLoop = this;
    wakeUpFd_ = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (wakeUpFd_ < 0)
        Logger::sys("eventfd error");
    auto wakeUpEvent = Event::make(wakeUpFd_, &epoller_);
    wakeUpEvent->setReadCallback([this] { readWakeUpFd(); });
    wakeUpEvent->setReadable(true);

    looping_ = true;
}

