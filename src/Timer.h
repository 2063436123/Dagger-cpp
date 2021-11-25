//
// Created by Hello Peter on 2021/11/19.
//

#ifndef TESTLINUX_TIMER_H
#define TESTLINUX_TIMER_H

#include <functional>
#include <sys/timerfd.h>
#include <Event.h>

class EventLoop;
class Timer;

class TimerHandler {
public:
    friend class Timer;
    explicit TimerHandler(int timerfd = 0, Event *event = nullptr) : timerfd_(timerfd), event_(event) {    }
    void resetOneTask(uint32_t time);
    void cancelTimer();
private:
    int timerfd_;
    Event* event_; // Timer生命期应长于TimerHandler
};

class Timer {
public:
    friend class TimerHandler;
    explicit Timer(EventLoop *);
    TimerHandler addOneTask(uint32_t time, const std::function<void()>& task);
    TimerHandler addTimedTask(uint32_t firstTime, uint32_t intervalTime, const std::function<void()>& task);
    ~Timer();

private:
    EventLoop *loop_;
};

#endif //TESTLINUX_TIMER_H
