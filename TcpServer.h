//
// Created by Hello Peter on 2021/9/7.
//

#pragma once

#include <map>
#include "Buffer.h"
#include "Socket.h"
#include "Event.h"
#include "InAddr.h"
#include "TcpConnection.h"
#include "EventLoop.h"
#include "EventLoopPool.h"

// 单线程TcpServer
class TcpServer {
    friend class TcpConnection;

public:
    // 在使用 右值变量时还是 当成左值来用（只要有名字就是左值）.
    // 此处可用Socket或Socket&&来接受右值.
    // C++ Primer P478: 因为listenfd 是一个非引用参数，所以对它进行拷贝初始化 -> 左值使用拷贝构造函数，右值使用移动构造函数
    TcpServer(Socket listenfd, InAddr addr, EventLoop *loop) : listenfd_(std::move(listenfd)), loop_(loop),
                                                               pool_(loop) {
        listenfd_.setReuseAddr();
        listenfd_.bindAddr(addr);
        listenfd_.setNonblock();

        loop_->init();
        auto event = Event::make(listenfd_.fd(), loop_->epoller());

        assert(event);
        event->setReadCallback(std::bind(&TcpServer::acceptCallback, this));
        event->setReadable(true);
    }

    void setConnMsgCallback(std::function<void(TcpConnection &)> callback) {
        connMsgCallback_ = callback;
    }

    void setConnEstaCallback(std::function<void(TcpConnection &)> callback) {
        connEstaCallback_ = callback;
    }

    void setConnCloseCallback(std::function<void(TcpConnection &)> callback) {
        connCloseCallback_ = callback;
    }

    // 启动EventLoop，开始监听listenfd和其他事件
    void start(size_t nums = 0) {
        listenfd_.listen(4000);
        // note pool_.setHelperThreadsNumAndStart() 必须先于 loop_->start()
        pool_.setHelperThreadsNumAndStart(nums);
        loop_->start();
    }

private:
    // for connfd
    // socket -> buffer(writable)
    // buffer(readable) -> user
    void preConnMsgCallback(TcpConnection &connection) {
        // 本函数应该：读取socket上数据到buffer，如果为0调用destroy()函数释放连接资源
        // 否则将buffer等作为参数，回调用户的readCallback.
        ssize_t n = connection.readBuffer().readFromSocket(connection.socket());
        if (n == 0) {
            // eof
            closeConnection(connection);
            return;
        }
        connMsgCallback_(connection);
    }

    // for listenfd_
    void acceptCallback() {
        int connfd = ::accept(listenfd_.fd(), nullptr, nullptr);
        if (connfd < 0) {
            if (errno == EAGAIN)
                return;
            Logger::sys("accept error");
        }
        auto ownerEventLoop = pool_.getNextPool();
        Logger::info("when accept, address of eventLoop: {} and the new connfd: {}\n", (void *) ownerEventLoop, connfd);
        auto connEvent = Event::make(connfd, ownerEventLoop->epoller());

        std::pair<int, TcpConnection> apair(connfd, TcpConnection::make(connfd, this, ownerEventLoop));

        std::unique_lock<std::mutex> ul(conns_mutex_);
        auto ret = connections_.insert(std::move(apair));
        ul.unlock();

        assert(ret.second);

        auto &newConn = ret.first->second;
        Logger::info("newConn: {}\n", (void *) &newConn);
        auto bindCallback = [this, &newConn]() {
            this->preConnMsgCallback(newConn);
        };
        connEvent->setReadCallback(bindCallback);
        connEvent->setReadable(true);
        // 回调EstablishedCallback()
        connEstaCallback_(newConn);
    }

    // todo 对connections_的insert和erase操作都应该由mainThread来完成，参考muduo runInLoop.
    void closeConnection(TcpConnection &connection) {
        // fixed: 确保数据被发送
        if (connection.writeBuffer().readableBytes() > 0) {
            auto event = connection.eventLoop()->epoller()->getEvent(connection.socket().fd());
            event->setReadable(false);
            connection.send(true);
            return;
        }
//        std::cout << "when close, address of eventLoop: " << connection.eventLoop() << std::endl;
        // 销毁前先调用CloseCallback
        connCloseCallback_(connection);
        connection.destroy();
        connection.eventLoop()->epoller()->removeEvent(connection.socket().fd());

        std::unique_lock<std::mutex> ul(conns_mutex_);
        connections_.erase(connection.socket().fd());
        ul.unlock();

        return;
    }

private:
    Socket listenfd_;
    EventLoop *loop_; // for listenfd
    EventLoopPool pool_;
    std::unordered_map<int, TcpConnection> connections_; // 接管 TcpConnection 生命期
    std::mutex conns_mutex_;
    std::function<void(TcpConnection &)> connMsgCallback_, connEstaCallback_, connCloseCallback_;
};

