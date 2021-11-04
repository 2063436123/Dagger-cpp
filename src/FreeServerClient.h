//
// Created by Hello Peter on 2021/10/27.
//

#ifndef TESTLINUX_FREESERVERCLIENT_H
#define TESTLINUX_FREESERVERCLIENT_H

#include <sys/timerfd.h>
#include "TcpConnection.h"
#include "InAddr.h"
#include "Socket.h"
#include "EventLoopPool.h"
#include "Codec.h"

#ifdef IDLE_CONNECTIONS_MANAGER
extern uint32_t timeInProcess; // counter for checking idle connections
#endif

const int REAL_SECONDS_PER_VIRTUAL_SECOND = 1;

// a server and client
class FreeServerClient : TcpSource {
public:
    // 在使用 右值变量时还是 当成左值来用（只要有名字就是左值）.
    // 此处可用Socket或Socket&&来接受右值.
    // C++ Primer P478: 因为listenfd 是一个非引用参数，所以对它进行拷贝初始化 -> 左值使用拷贝构造函数，右值使用移动构造函数
    FreeServerClient(EventLoop *loop) : loop_(loop), pool_(loop) {
        loop_->init();
#ifdef IDLE_CONNECTIONS_MANAGER
        createIdleConnTimer();
#endif
    }

    using Func = std::function<void(TcpConnection *)>;

    void createServerPort(InAddr addr, Codec codec, Func establishedCallback, Func messageCallback, Func destoryCallback,
                          Func errorCallback) {
        serverfds_.push_back(Socket::makeNewSocket());
        auto &listenfd = serverfds_.back();
        listenfd.setReuseAddr();
        listenfd.bindAddr(addr);
        listenfd.setNonblock();
        listenfd.listen(4096);

        // todo 使用哪个loop？
        auto event = Event::make(listenfd.fd(), loop_->epoller());
        event->setReadCallback(std::bind(&FreeServerClient::acceptCallback, this, listenfd.fd(), codec, establishedCallback,
                                         messageCallback, destoryCallback, errorCallback));
        event->setReadable(true);
    }

    TcpConnection* createClientConn(InAddr addr, Codec codec, Func establishedCallback, Func messageCallback, Func destoryCallback,
                          Func errorCallback) {
        // todo 暂时用阻塞式connect连接
        // clientfd.setNonblock();
        clientfds_.push_back(Socket::makeNewSocket());
        auto& clientfd = clientfds_.back();
        clientfd.connect(addr);

        // todo 使用哪个loop？
        auto event = Event::make(clientfd.fd(), loop_->epoller());
        auto conn = TcpConnection::makeHeapObject(event, this, loop_);
        if (establishedCallback)
            establishedCallback(conn);

        auto preConnMsgCallback = [conn, codec = std::move(codec), messageCallback, destoryCallback, errorCallback, this]() {
            ssize_t n = conn->readBuffer().readFromSocket(conn->socket());
            if (n == 0) {
                // 对端关闭
                closeConnection(conn, destoryCallback);
                return;
            } else if (n < 0) {
                if (errorCallback)
                    errorCallback(conn);
                Logger::sys("read error");
                return;
            }
            if (messageCallback) {
                // todo: 添加codec处理
                if (codec.check(conn))
                    messageCallback(conn);
            }
        };
        event->setReadCallback(preConnMsgCallback);
        event->setReadable(true);

        return conn;
    }

    // 单位是ms
    void addTimedTask(uint32_t nextOccurTime, uint32_t intervalTime, std::function<void()> task) {
        timespec nxtTime{.tv_sec = nextOccurTime / 1000, .tv_nsec = (nextOccurTime % 1000) * 1000000};
        timespec interTime{.tv_sec = intervalTime / 1000, .tv_nsec = (intervalTime % 1000) * 1000000};
        struct itimerspec spec{.it_interval = interTime, .it_value = nxtTime};
        int timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
        if (timerfd_settime(timerfd, 0, &spec, nullptr) < 0)
            Logger::sys("timerfd_settime error");

        auto timerEvent = Event::make(timerfd, pool_.getNextPool()->epoller());
        auto readCallback = [timerfd = timerfd, task]() {
            uint64_t tmp;
            read(timerfd, &tmp, sizeof(tmp));
            task();
        };
        timerEvent->setReadCallback(readCallback);
        timerEvent->setReadable(true);
    }

    // 启动EventLoop，开始监听listenfd和其他事件
    void start(size_t nums = 0) {
        Logger::info("FreeServerClient starting...\n");
        // note pool_.setHelperThreadsNumAndStart() 必须先于 loop_->start()
        pool_.setHelperThreadsNumAndStart(nums);
        loop_->start();
    }

    void stop() {
        loop_->stop();
        // pool_.~EventLoopPool();
    }

private:
#ifdef IDLE_CONNECTIONS_MANAGER
    void createIdleConnTimer() {
        int timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
        // 每秒触发一次
        struct itimerspec spec{
                .it_interval = timespec{.tv_sec = REAL_SECONDS_PER_VIRTUAL_SECOND, .tv_nsec = 0},
                .it_value = timespec{.tv_sec = 0, .tv_nsec = 0}};
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

    // for listenfd_, 此函数只会在main thread中执行，所以无race condition
    void acceptCallback(int listenfd, Codec codec, Func establishedCallback, Func messageCallback, Func destoryCallback,
                        Func errorCallback) {
        int connfd = ::accept(listenfd, nullptr, nullptr);
        if (connfd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EPROTO || errno == ECONNABORTED)
                return;
            Logger::sys("accept error");
        }

        auto ownerEventLoop = pool_.getNextPool();
        auto connEvent = Event::make(connfd, ownerEventLoop->epoller());

        // 创建TcpConnection时自动设置NONBLOCK标志
        auto newConn = TcpConnection::makeHeapObject(connEvent, this, ownerEventLoop);
        newConn->setDestoryCallback(destoryCallback);
#ifdef IDLE_CONNECTIONS_MANAGER
        newConn->lastReceiveTime = timeInProcess;
        wheelPolicy_.addNewConnection(newConn);
#endif
        auto bindCallback = [connection = newConn, codec = std::move(codec), messageCallback, destoryCallback, errorCallback, this]() {
            ssize_t n = connection->readBuffer().readFromSocket(connection->socket());
            if (n == 0) {
                // eof
                closeConnection(connection, destoryCallback);
                return;
            } else if (n < 0) {
                errorCallback(connection);
                Logger::sys("readFromSocket error");
            }
#ifdef IDLE_CONNECTIONS_MANAGER
            connection->lastReceiveTime = timeInProcess;
#endif
            if (codec.check(connection))
                messageCallback(connection);
        };
        connEvent->setReadCallback(bindCallback);
        connEvent->setReadable(true);
        // 回调EstablishedCallback()
        if (establishedCallback)
            establishedCallback(newConn);
    }

    // todo 对connections_的insert和erase操作都应该由mainThread来完成，参考muduo runInLoop.
    void closeConnection(TcpConnection *connection, Func destoryCallback) override {
        // fixed: 确保数据被发送
        if (connection->writeBuffer().readableBytes() > 0) {
            auto event = connection->event();
            event->setReadable(false);
            connection->send(true);
            return;
        }
        // 销毁前先调用destoryCallback
        if (destoryCallback)
            destoryCallback(connection);
        connection->destroy();

        // todo 释放connection资源，由wheeling来（定时而非及时）释放资源哈哈
#ifndef IDLE_CONNECTIONS_MANAGER
        delete connection;
#endif

        return;
    }

private:
    // todo 并发问题？各种fd能否随机分配EventLoopPool中的EventLoop而不产生线程安全问题？
    std::vector<Socket> serverfds_, clientfds_;
    EventLoop *loop_; // for listenfd
    EventLoopPool pool_;
#ifdef IDLE_CONNECTIONS_MANAGER
    TimerWheelingPolicy wheelPolicy_; // 提供一种策略，tcpserver每秒调用一次该策略来删除某些超时连接
#endif
};

#endif //TESTLINUX_FREESERVERCLIENT_H
