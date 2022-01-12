//
// Created by Hello Peter on 2021/9/7.
//

#ifndef TESTLINUX_TCPSERVER_H
#define TESTLINUX_TCPSERVER_H

//#define IDLE_CONNECTIONS_MANAGER

#include <map>
#include <sys/timerfd.h>
#include "Codec.h"
#include "Timer.h"
#include "Buffer.h"
#include "Socket.h"
#include "Event.h"
#include "InAddr.h"
#include "TcpConnection.h"
#include "EventLoop.h"
#include "EventLoopPool.h"
#include "TimeWheeling.h"
#include "TcpSource.h"

#ifdef IDLE_CONNECTIONS_MANAGER
extern uint32_t timeInProcess; // counter for checking idle connections
#endif

ObjectPool &getObjectPool();

const int REAL_SECONDS_PER_VIRTUAL_SECOND = 1;

#include <csignal>
static void sigpipe_handler(int sig) {
    assert(sig == SIGPIPE);
}
static void shield_sigpipe() {
    signal(SIGPIPE, sigpipe_handler);
}

extern thread_local EventLoop *localEventLoop;
// 单线程TcpServer
class TcpServer : public TcpSource {
    friend class TcpConnection;

public:
    // 在使用 右值变量时还是 当成左值来用（只要有名字就是左值）.
    // 此处可用Socket或Socket&&来接受右值.
    // C++ Primer P478: 因为listenfd 是一个非引用参数，所以对它进行拷贝初始化 -> 左值使用拷贝构造函数，右值使用移动构造函数
    TcpServer(Socket listenfd, InAddr addr, EventLoop *loop, Codec codec) : listenfd_(std::move(listenfd)), loop_(loop),
                                                                            pool_(loop), codec_(std::move(codec)),
                                                                            timer_(loop) {
        shield_sigpipe();
        // for performance
        if (Options::setMaxFiles(1048576) < 0)
            Logger::sys("getMaxFiles error");

        listenfd_.setReuseAddr();
        listenfd_.setReusePort();
        listenfd_.bindAddr(addr);
        listenfd_.setNonblock();

        loop_->init();
        auto event = Event::make(listenfd_.fd(), loop_->epoller());

        assert(event);
        event->setReadCallback(std::bind(&TcpServer::acceptCallback, this));
        event->setReadable(true);
        listenEvent_ = event;

#ifdef IDLE_CONNECTIONS_MANAGER
        createIdleConnTimer();
#endif
    }

    void setConnMsgCallback(std::function<void(TcpConnection *)> callback) {
        connMsgCallback_ = callback;
    }

    void setConnEstaCallback(std::function<void(TcpConnection *)> callback) {
        connEstaCallback_ = callback;
    }

    void setConnCloseCallback(std::function<void(TcpConnection *)> callback) {
        connCloseCallback_ = callback;
    }

    // 单位是ms
    std::shared_ptr<TimerHandler> addOneTask(uint32_t time, const std::function<void()>& task) {
        return timer_.addOneTask(time, task);
    }

    // 单位是ms
    std::shared_ptr<TimerHandler> addTimedTask(uint32_t firstTime, uint32_t intervalTime, const std::function<void()>& task) {
        return timer_.addTimedTask(firstTime, intervalTime, task);
    }

    void cancelTimer(TimerHandler handler) {
        handler.cancelTimer();
    }

    // 启动EventLoop，开始监听listenfd和其他事件
    void start(size_t nums = 0) {
        std::cout << ("TcpServer starting...\n") << std::endl;
        listenfd_.listen(4096);
        // note pool_.setHelperThreadsNumAndStart() 必须先于 loop_->start()
        pool_.setHelperThreadsNumAndStart(nums);
        loop_->start();
    }

    void stop() {
        loop_->stop();
        // pool_.~EventLoopPool();
        delete listenEvent_;
    }

private:
#ifdef IDLE_CONNECTIONS_MANAGER
    void createIdleConnTimer() {
        int timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
        // 每秒触发一次
        struct itimerspec spec{
                .it_interval = timespec{.tv_sec = REAL_SECONDS_PER_VIRTUAL_SECOND, .tv_nsec = 0},
                .it_value = timespec{.tv_sec = 1, .tv_nsec = 0}};
        timerfd_settime(timerfd, 0, &spec, nullptr);

        // 计时器回调
        auto readCallback = [timerfd, this]() {
            uint64_t tmp;
            read(timerfd, &tmp, sizeof(tmp)); // consume
            ++timeInProcess; // 每秒加一
            this->wheelPolicy_.eraseIdleConnections();
        };

        auto timerEvent = Event::make(timerfd, loop_->epoller());
        timerEvent->setReadCallback(readCallback);
        timerEvent->setReadable(true);
    }
#endif

    // for connfd
    // socket -> buffer(writable)
    // buffer(readable) -> user
    void preConnMsgCallback(TcpConnection *connection) {
        connection->setData(localEventLoop); // fixme 验证closeConnection和preConnMsgCallback被同一个线程调用
        // 本函数应该：读取socket上数据到buffer，如果为0调用destroy()函数释放连接资源
        // 否则将buffer等作为参数，回调用户的readCallback.
        ssize_t n = connection->readBuffer().readFromSocket(connection->socket());
        if (n == 0) {
            // eof
            closeConnection(connection, nullptr);
            return;
        } else if (n < 0) {
            Logger::sys("read from socket error");
            closeConnection(connection, nullptr);
            return;
        }
#ifdef IDLE_CONNECTIONS_MANAGER
        connection->lastReceiveTime = timeInProcess;
#endif
        // todo: 添加codec处理
        if (codec_.check(connection))
            connMsgCallback_(connection);
    }

    // for listenfd_, 此函数只会在main thread中执行，所以无race condition
    void acceptCallback() {
        int connfd;
        do {
            // 尽可能多地在一次accept回调中收集新连接
            connfd = listenfd_.accept();
            if (connfd <= 0)
                break;
            auto ownerEventLoop = pool_.getNextPool();
            auto connEvent = Event::make(connfd, ownerEventLoop->epoller());

            // 创建TcpConnection时自动设置NONBLOCK标志
            auto newConn = TcpConnection::makeHeapObject(connEvent, this, ownerEventLoop);
#ifdef IDLE_CONNECTIONS_MANAGER
            newConn->lastReceiveTime = timeInProcess;
            wheelPolicy_.addNewConnection(newConn);
#endif

            auto bindCallback = [this, newConn]() {
                this->preConnMsgCallback(newConn);
            };
            connEvent->setReadCallback(bindCallback);
            connEvent->setReadable(true);
            // 回调EstablishedCallback()
            if (connEstaCallback_)
                connEstaCallback_(newConn);
        } while (true);
    }

    // todo 对connections_的insert和erase操作都应该由mainThread来完成，参考muduo runInLoop.
    void closeConnection(TcpConnection *connection, std::function<void(TcpConnection *)> destoryCallback) override {
        assert (connection->data() == localEventLoop); // fixme 验证closeConnection和preConnMsgCallback被同一个线程调用
        assert(destoryCallback == nullptr);
        // fixed: 确保数据被发送
        if (connection->writeBuffer().readableBytes() > 0) {
            auto event = connection->event();
            event->setReadable(false);
            connection->send(true);
            return;
        }
        // 销毁前先调用CloseCallback
        if (connCloseCallback_)
            connCloseCallback_(connection);
        connection->destroy();

        // todo 释放connection资源，由wheeling来（定时而非及时）释放资源哈哈
#ifndef IDLE_CONNECTIONS_MANAGER
        //delete connection;
        // todo change delete to recovery
        ObjectPool::returnObject<TcpConnection>(connection);
#endif
    }

private:
    Socket listenfd_;
    Event *listenEvent_; // to be deleted
    EventLoop *loop_; // for listenfd
    EventLoopPool pool_;
    Codec codec_;
    Timer timer_;
#ifdef IDLE_CONNECTIONS_MANAGER
    TimerWheelingPolicy wheelPolicy_; // 提供一种策略，tcpserver每秒调用一次该策略来删除某些超时连接
#endif
    std::function<void(TcpConnection *)> connMsgCallback_, connEstaCallback_, connCloseCallback_;
};

#endif