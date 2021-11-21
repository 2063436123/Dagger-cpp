//
// Created by Hello Peter on 2021/10/20.
//
#include "../Timer.h"
#include "../Logger.h"
#include "../Event.h"
#include "../EventLoop.h"

Timer::Timer(EventLoop *loop) : loop_(loop) {}

Timer::~Timer() {}

TimerHandler Timer::addTimedTask(uint32_t firstTime, uint32_t intervalTime, const std::function<void()> &task) {
    if (firstTime == 0)
        firstTime = 1;
    timespec nxtTime{.tv_sec = firstTime / 1000, .tv_nsec = (firstTime % 1000) * 1000000};
    timespec interTime{.tv_sec = intervalTime / 1000, .tv_nsec = (intervalTime % 1000) * 1000000};
    struct itimerspec spec{.it_interval = interTime, .it_value = nxtTime};
    int timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (timerfd_settime(timerfd, 0, &spec, nullptr) < 0)
        Logger::sys("timerfd_settime error");

    // fixme 内存泄漏，timerEvent
    auto timerEvent = Event::make(timerfd, loop_->epoller());
    auto readCallback = [timerfd = timerfd, task]() {
        uint64_t tmp;
        read(timerfd, &tmp, sizeof(tmp));
        task();
    };
    timerEvent->setReadCallback(readCallback);
    timerEvent->setReadable(true);
    return TimerHandler(timerfd, timerEvent);
}

TimerHandler Timer::addOneTask(uint32_t occurTime, const std::function<void()> &task) {
    timespec nxtTime{.tv_sec = occurTime / 1000, .tv_nsec = (occurTime % 1000) * 1000000};
    timespec interTime{.tv_sec = 0, .tv_nsec = 0};
    struct itimerspec spec{.it_interval = interTime, .it_value = nxtTime};
    int timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (timerfd_settime(timerfd, 0, &spec, nullptr) < 0)
        Logger::sys("timerfd_settime error");

    auto epoller = loop_->epoller();
    auto timerEvent = Event::make(timerfd, epoller);
    auto readCallback = [timerfd = timerfd, task, timerEvent, epoller]() {
        uint64_t tmp;
        read(timerfd, &tmp, sizeof(tmp));
        task();
        // timer到期
        epoller->removeEvent(timerEvent);
    };
    timerEvent->setReadCallback(readCallback);
    timerEvent->setReadable(true);
    return TimerHandler(timerfd, timerEvent);
}

void TimerHandler::resetOneTask(uint32_t time) {
    int timerfd = timerfd_;
    timespec nxtTime{.tv_sec = time / 1000, .tv_nsec = (time % 1000) * 1000000};
    timespec interTime{.tv_sec = 0, .tv_nsec = 0};
    struct itimerspec spec{.it_interval = interTime, .it_value = nxtTime};
    if (timerfd_settime(timerfd, 0, &spec, nullptr) < 0)
        Logger::sys("timerfd_settime error");
}

void TimerHandler::cancelTimer() {
    timespec nxtTime{.tv_sec = 0, .tv_nsec = 0};
    timespec interTime{.tv_sec = 0, .tv_nsec = 0};
    struct itimerspec spec{.it_interval = interTime, .it_value = nxtTime};
    int timerfd = timerfd_;
    if (timerfd_settime(timerfd, 0, &spec, nullptr) < 0)
        Logger::sys("timerfd_settime error");
    // 销毁timerfd
    event_->epoller()->removeEvent(event_);
    close(timerfd_);
}
