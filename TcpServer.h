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

// 单线程TcpServer
class TcpServer {
public:
    // 在使用 右值变量时还是 当成左值来用（只要有名字就是左值）.
    // 此处可用Socket或Socket&&来接受右值.
    // C++ Primer P478: 因为listenfd 是一个非引用参数，所以对它进行拷贝初始化 -> 左值使用拷贝构造函数，右值使用移动构造函数
    TcpServer(Socket listenfd, InAddr addr) : listenfd_(std::move(listenfd)) {
        listenfd_.bindAddr(addr);
        listenfd_.setReuseAddr();
        auto event = Event::make(listenfd_.fd(), &epoller_);
        assert(event);
        event->setReadCallback(std::bind(&TcpServer::readCallback, this));
        event->setReadable(true);
    }

    void setConnMsgCallback(std::function<void(TcpConnection&)> callback) {
        connMsgCallback_ = callback;
    }

    void setConnEstaCallback(std::function<void(TcpConnection&)> callback) {
        connEstaCallback_ = callback;
    }

    void setConnCloseCallback(std::function<void(TcpConnection&)> callback) {
        connCloseCallback_ = callback;
    }

    void start() {
        listenfd_.listen(4000);
        while (true) {
            auto ret = epoller_.poll();
            for (auto& event : ret)
                event->handle();
        }
    }

private:
    // for connfd
    // socket -> buffer(writable)
    // buffer(readable) -> user
    void preConnMsgCallback(TcpConnection& connection) {
        // 本函数应该做：读取socket上数据到buffer，如果为0调用destroy()函数释放连接资源
        // 否则将buffer等作为参数，回调用户的readCallback.
        ssize_t n = connection.readBuffer().readFromSocket(connection.socket());
        if (n == 0) {
            // eof
            // 销毁前先调用CloseCallback
            connCloseCallback_(connection);
            connection.destroy();
            epoller_.removeEvent(connection.socket().fd());
            connections_.erase(connection.socket().fd());
            return;
        }
        connMsgCallback_(connection);
    }

    // for listenfd_
    void readCallback() {
        std::cout << "readCallback()::accept occur!" << std::endl;
        int connfd = ::accept(listenfd_.fd(), nullptr, nullptr);
        auto connEvent = Event::make(connfd, &epoller_);

        // fixme 无法声明默认的析构函数，否则会引起TcpServer::readCallback()函数中的connections.insert插入错误！
        std::pair<int, TcpConnection> apair(connfd, TcpConnection::make(connfd, &epoller_));
        connections_.insert(std::move(apair));
        auto bindCallback = [this, connfd]() {
            this->preConnMsgCallback(connections_.find(connfd)->second);
        };
        connEvent->setReadCallback(bindCallback);
        connEvent->setReadable(true);
        // 回调EstablishedCallback()
        connEstaCallback_(connections_.find(connfd)->second);
    }

private:
    Socket listenfd_;
    Epoller epoller_; // 生命期要长于所有的Event
    std::unordered_map<int, TcpConnection> connections_; // 接管 TcpConnection 生命期
    std::function<void(TcpConnection&)> connMsgCallback_, connEstaCallback_, connCloseCallback_;
};

