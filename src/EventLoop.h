//
// Created by Hello Peter on 2021/9/12.
//

#ifndef TESTLINUX_EVENTLOOP_H
#define TESTLINUX_EVENTLOOP_H

#include <thread>
#include <mutex>
#include <iostream>
#include "Event.h"
#include "SpinLock.h"
#include <sys/stat.h>
#include <atomic>

class EventLoop;
using MutexLock = std::mutex;

class EventLoop {
public:
    EventLoop();

    // 每个EventLoop的init函数必须在不同的线程中分别被调用.
    void init();

    void start() {
        while (looping_) {
            auto ret = epoller_.poll();
            for (auto &event: ret) {
                event->handle();
            }

            isInTaskHanding_ = true;
            // create tmp + swap with lock
            std::vector<std::function<void()>> curTask;
            std::unique_lock<MutexLock> lg(lock_);
            curTask.swap(tasks_);
            assert(tasks_.empty());
            lg.unlock();

            for (const auto &task: curTask)
                task();
            isInTaskHanding_ = false;
        }
    }

    Epoller *epoller() {
        return &epoller_;
    }

    void stop() {
        looping_ = false;
        writeWakeUpFd();
        // do sth...
    }

    void readWakeUpFd() const {
        uint64_t tmp;
        read(wakeUpFd_, &tmp, sizeof(tmp));
    }

    void writeWakeUpFd() const {
        uint64_t tmp;
        write(wakeUpFd_, &tmp, sizeof(tmp));
    }

    void runInLoop(std::function<void()> task) {
        // runInLoop 目前还不需要加锁，因为只有main thread会每一秒调用一次该函数
        {
            std::lock_guard<MutexLock> lg(lock_);
            tasks_.push_back(std::move(task));
        }
        // 打断eopller.poll()，否则在没有新事件到来时，这些task将饥饿
        if (!isInTaskHanding_) {
            // wakeup
            writeWakeUpFd();
        }
    }

private:
    Epoller epoller_; // 生命期要长于所有的Event
    std::vector<std::function<void()>> tasks_;
    int wakeUpFd_; // 在runInLoop添加新任务后唤醒epoller_
    std::atomic<bool> isInTaskHanding_; // 如果已经在处理tasks，那么就不需要冗余地唤醒epoller_
    MutexLock lock_;
    bool looping_;
};


#endif //TESTLINUX_EVENTLOOP_H
