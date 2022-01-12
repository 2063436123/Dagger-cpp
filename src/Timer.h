//
// Created by Hello Peter on 2021/11/19.
//

#ifndef TESTLINUX_TIMER_H
#define TESTLINUX_TIMER_H

#include <functional>
#include <sys/timerfd.h>
#include "Event.h"

class EventLoop;

class Timer;

class TimerHandler : std::enable_shared_from_this<TimerHandler> {
public:
    friend class Timer;
    void resetOneTask(uint32_t time);
    void cancelTimer();
private:
    TimerHandler(int timerfd, Event* event, Timer *timer) : timerfd_(timerfd), event_(event), timer_(timer) {}
    int timerfd_;
    Event* event_;
    Timer *timer_;
};

// Timer生命期应长于TimerHandler
class Timer {
public:
    explicit Timer(EventLoop *);

    std::shared_ptr<TimerHandler> addOneTask(uint32_t time, const std::function<void()> &task);

    std::shared_ptr<TimerHandler>
    addTimedTask(uint32_t firstTime, uint32_t intervalTime, const std::function<void()> &task);

    void removeTimer(std::shared_ptr<TimerHandler> timerHandler);

    ~Timer();

    class ConcurrentMap {
    public:
        void add(std::shared_ptr<TimerHandler> timerHandler) {
            std::lock_guard<std::mutex> lg(mu_);
            um_.insert({timerHandler->timerfd_, timerHandler});
        }

        void remove(std::shared_ptr<TimerHandler> timerHandler) {
            std::lock_guard<std::mutex> lg(mu_);
            um_.erase(timerHandler->timerfd_);
        }

        ~ConcurrentMap() {
            for (auto& handler : um_) {
                handler.second->cancelTimer();
            }
        }
    private:
        std::unordered_map<int, std::shared_ptr<TimerHandler>> um_;
        std::mutex mu_; // TODO 如果一个Timer只被一个线程使用，那么不需要加锁
    };
private:
    ConcurrentMap timer_handler_queue_; // 析构时自动销毁所有定时器
    EventLoop *loop_;
};

#endif //TESTLINUX_TIMER_H
