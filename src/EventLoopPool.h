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
    explicit EventLoopPool(EventLoop *single_loop) : single_loop_(single_loop), nextIndex_(0),
                                                     loops_(MAX_THREADS), realSize_(0) {
        // note EventLoop的上限由MAX_THREADS决定
//        loops_.resize(MAX_THREADS); // 如果loops_的元素是不可移动（同时也不可拷贝）的，那么只能在构造函数中分配数组大小
        threads_.reserve(MAX_THREADS);
    }

    EventLoop *getNextPool() {
        if (threads_.empty())
            return single_loop_;
        // 大致公平的算法
        size_t nextIndex = nextIndex_++ % realSize_;
        return &loops_[nextIndex];
    }

    void threadFunc(int i) {
//        std::unique_lock<std::mutex> ul(threadInitMutex_);
        auto &loop = loops_[i];
        // 每个线程直接运行EventLoop::start()，其中会执行Epoller::poll()，阻塞在epoll_wait上；
        // 得益于epoll对并发的支持，single_loop_可以在某一Epoller阻塞时向其中添加新的监听事件。
//        ul.unlock();
        loop.init();
        std::unique_lock<std::mutex> ul(mu_);
        ++cnt;
        cv_.notify_one();
        ul.unlock();
        loop.start();
    }

    void setHelperThreadsNumAndStart(size_t nums = 0) {
        assert(nums <= MAX_THREADS);
        realSize_ = nums;
        for (size_t i = 0; i < nums; i++) {
            threads_.emplace_back(&EventLoopPool::threadFunc, this, i);
        }
        // todo: sleep是为了保证在此函数返回时，所有线程已创建完毕且运行至EventLoop::start()，但应该使用条件变量代替
        //sleep(1);
        std::unique_lock<std::mutex> ul(mu_);
        cv_.wait(ul, [this, nums]() { return cnt == nums; });
    }

    ~EventLoopPool() {
        for (auto &aLoop: loops_)
            aLoop.stop();
        for (auto &aThread: threads_)
            aThread.join();
    }

private:
    std::mutex threadInitMutex_;
    // 保存主EventLoop，不设置多线程时，TcpServer的listenfd和connfd共用这一个single_loop_
    EventLoop *single_loop_;
    // 每个线程对应一个EventLoop
    std::vector<std::thread> threads_;
    std::vector<EventLoop> loops_;
    // 保证setHelperThreadsNumAndStart返回时线程都在运行
    std::mutex mu_;
    std::condition_variable cv_;
    std::atomic<std::size_t> cnt = 0;

    // 下一个被选中的EventLoop
    size_t nextIndex_;
    size_t realSize_;
};


#endif //TESTLINUX_EVENTLOOPPOOL_H
