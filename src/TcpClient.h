//
// Created by Hello Peter on 2021/10/22.
//

#ifndef TESTLINUX_TCPCLIENT_H
#define TESTLINUX_TCPCLIENT_H

#include "InAddr.h"
#include "Logger.h"
#include "Socket.h"
#include "Buffer.h"
#include "Event.h"
#include "TcpConnection.h"
#include "TcpSource.h"
#include "EventLoopPool.h"

class TcpClient : public TcpSource {
public:
    // 一个实例对应一个客户
    TcpClient(Socket clientfd, EventLoop *loop) : clientfd_(std::move(clientfd)), loop_(loop) {
        loop_->init();
    }

    void setConnMsgCallback(std::function<void(TcpConnection *)> callback) {
        connMsgCallback_ = callback;
    }

    void setConnCloseCallback(std::function<void(TcpConnection *)> callback) {
        connCloseCallback_ = callback;
    }

    void setConnErrorCallback(std::function<void(TcpConnection*)> callback) {
        connErrorCallback_ = callback;
    }

    TcpConnection* connect(InAddr addr) {
        // todo 暂时用阻塞式connect连接
        // clientfd.setNonblock();
        clientfd_.connect(addr);

        auto event = Event::make(clientfd_.fd(), loop_->epoller());
        auto conn = TcpConnection::makeHeapObject(clientfd_.fd(), this, loop_);
        auto preConnMsgCallback = [conn, this]() {
            ssize_t n = conn->readBuffer().readFromSocket(conn->socket());
            if (n == 0) {
                // 对端关闭
                closeConnection(conn, nullptr);
                return;
            } else if (n < 0) {
                connErrorCallback_(conn);
                Logger::sys("read error");
                closeConnection(conn, nullptr);
                return;
            }
            connMsgCallback_(conn);
        };
        event->setReadCallback(preConnMsgCallback);
        event->setReadable(true);

        backThread_ = std::thread(&EventLoop::start, loop_);
        return conn;
    }

    // 关闭连接，在 read返回0 或 客户调用activeClose 时被调用
    void closeConnection(TcpConnection *connection, std::function<void(TcpConnection*)> destoryCallback) override {
        assert(destoryCallback == nullptr);
        if (connection->writeBuffer().readableBytes() > 0) {
            auto event = connection->eventLoop()->epoller()->getEvent(connection->socket().fd());
            event->setReadable(false);
            connection->send(true);
            return;
        }
        // 销毁前先调用CloseCallback
        connCloseCallback_(connection);
        connection->destroy();
        // TcpConnection析构时调用Socket成员的析构函数，其中会close(sockfd)完成连接的关闭
        delete connection;

        // FIXME: 假设唯一的一条连接断开时自动终止eventLoop
        loop_->stop();
        return;
    }

    void join() {
        backThread_.join();
    }

    ~TcpClient() {
        if (backThread_.joinable())
            backThread_.join();
    }

private:
    Socket clientfd_;
    EventLoop *loop_;
    std::thread backThread_; // 副线程运行EventLoop
    std::function<void(TcpConnection *)> connMsgCallback_, connCloseCallback_, connErrorCallback_;

};

#endif //TESTLINUX_TCPCLIENT_H
