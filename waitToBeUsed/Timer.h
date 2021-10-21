//
// Created by Hello Peter on 2021/9/2.
//

#pragma once

#include <iostream>
#include <string>
#include <set>
#include <any>
#include <chrono>
#include <functional>
#include <future>
#include <atomic>
#include <vector>
#include <sys/time.h>
#include <unistd.h>


// 表示一个定时事件，时间单位都是ms
class Timer {
public:
    using callbackType = std::function<void(std::any)>;

    Timer(callbackType callback, uint64_t expirationTime, uint64_t intervalTime, std::any data)
            : callback_(callback), expirationTime_(expirationTime), intervalTime_(intervalTime), data_(data) {
        id_ = ++nextId_;
    }

    // set使用此operator<来做大小比较和相等性判断（当且仅当!(a<b）&&!(b<a)时认为ab相等）
    // 故无法对仅依靠过期时间即作大小判断又判断相等性。故应采用multiset
    bool operator<(const Timer &t) const {
        return this->getExpirationTime() < t.getExpirationTime();
    }

    uint32_t id() const {
        return id_;
    }

    std::any data() const {
        return data_;
    }

    uint64_t timeToNext() const {
        auto now = nowTime();
        if (now >= expirationTime_)
            return 0;
        return expirationTime_ - now;
    }

    uint64_t getExpirationTime() const {
        return expirationTime_;
    }

    void callback() {
        callback_(data_);
    }

    void bgCallback() {
        std::async(&Timer::callback, this);
    }

    bool repeat() const {
        return intervalTime_ != 0;
    }

    void addInterval() {
        auto now = nowTime();
        expirationTime_ = now + intervalTime_;
    }

    static uint64_t nowTime() {
        timeval now;
        gettimeofday(&now, nullptr);
        // clog << "sec: " << now.tv_sec << ", usec: " << now.tv_usec << endl;
        return now.tv_sec * 1000 + now.tv_usec / 1000;
    }

private:
    static std::atomic_uint32_t nextId_;
    uint32_t id_;
    std::any data_;
    uint64_t expirationTime_;
    uint64_t intervalTime_;
    callbackType callback_;
};

//std::atomic_uint32_t Timer::nextId_ = 0;

//
//class TimerQueue {
//public:
//    TimerQueue() {}
//
//    void addTimer(const Timer &timer) {
//        // fixme 如果Timer::operator<不定义为const的，那么insert将报错！
//        auto iter = timers_.insert(timer);
//    }
//
//    void removeTimer(uint32_t timerId) {
//        auto iter = timers_.begin();
//        timers_.erase(iter);
//    }
//
//    // 主函数，无尽循环
//    void eventloop() {
//        while (true) {
//            // asm volatile( "rep;nop": : : "memory"); // slow cpu spin
//            if (!timers_.empty()) {
//                auto iter = timers_.begin();
//                auto timer = *iter;
//                timers_.erase(iter);
//                uint64_t leftTime = timer.timeToNext();
//                usleep(leftTime * 1000);
//                timer.bgCallback();
//                if (timer.repeat()) {
//                    timer.addInterval();
//                    timers_.insert(timer);
//                }
//            }
//        }
//    }
//
//private:
//    std::multiset<Timer> timers_;
//
//};

//int main() {
//    // todo 判断sizeof(Timer)和sizeof各字段的关系。
//    cout << sizeof(std::function<void(Timer*)>) << " " << sizeof(any) << " " << sizeof(Timer)
//        << " " << sizeof(TimerQueue) << endl;
//    Timer timer1(show1, Timer::nowTime(), 1000, nullptr);
//    Timer timer2(show2, Timer::nowTime(), 500, nullptr);
//    Timer timer3(show3, Timer::nowTime(), 10, nullptr);
//    cout << timer1.id() << " " << timer2.id() << endl;
//    TimerQueue tq;
//    tq.addTimer(timer1);
//    tq.addTimer(timer2);
//    tq.addTimer(timer3);
//    tq.eventloop();
//}