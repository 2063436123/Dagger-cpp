//
// Created by Hello Peter on 2021/9/12.
//

#ifndef TESTLINUX_EVENTLOOPPOOL_H
#define TESTLINUX_EVENTLOOPPOOL_H

#include <unistd.h>
#include "EventLoop.h"

const int MAX_THREADS = std::thread::hardware_concurrency() * 2; // 64-bit macos = 8

class EventLoopPool {
public:
    EventLoopPool(EventLoop *single_loop) : single_loop_(single_loop), nextIndex_(0) {
        // note EventLoop的上限由此vector的大小决定
        loops_.reserve(MAX_THREADS);
        threads_.reserve(MAX_THREADS);
    }

    EventLoop *getNextPool() {
        if (threads_.empty())
            return single_loop_;
        // 大致公平的算法
        return &loops_[nextIndex_++ % loops_.size()];
    }

    void threadFunc() {
        std::unique_lock<std::mutex> ul(threadInitMutex_);
        // fixed 问题在于loops_增长时会移动原来的元素，那么对它的this引用就会失效。
        auto &loop = loops_.emplace_back(); // 线索，拷贝/移动未定义的问题吗？
        // 每个线程直接运行EventLoop::start()，其中会执行Epoller::poll()，阻塞在epoll_wait上；
        // 得益于epoll对并发的支持，single_loop_可以在某一Epoller阻塞时向其中添加新的监听事件。
        ul.unlock();
        loop.init();
        loop.start();
    }

    void setHelperThreadsNumAndStart(size_t nums = 0) {
        assert(nums <= MAX_THREADS);
        for (size_t i = 0; i < nums; i++) {
            threads_.emplace_back(&EventLoopPool::threadFunc, this);
        }
        // todo: sleep是为了保证在此函数返回时，所有线程已创建完毕且运行至EventLoop::start()，但应该使用条件变量代替
        sleep(2);
    }

    ~EventLoopPool() {
        for (auto &aLoop: loops_)
            aLoop.stop();
        for (auto &aThread : threads_)
            aThread.join();
    }

private:
    std::mutex threadInitMutex_;
    // 保存主EventLoop，不设置多线程时，TcpServer的listenfd和connfd共用这一个single_loop_
    EventLoop *single_loop_;
    // 每个线程对应一个EventLoop
    std::vector<std::thread> threads_;
    std::vector<EventLoop> loops_;
    // 下一个被选中的EventLoop
    size_t nextIndex_;
};


#endif //TESTLINUX_EVENTLOOPPOOL_H
