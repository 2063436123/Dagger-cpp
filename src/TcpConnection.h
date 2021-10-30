//
// Created by Hello Peter on 2021/9/11.
//

#ifndef TESTLINUX_TCPCONNECTION_H
#define TESTLINUX_TCPCONNECTION_H

#include "Buffer.h"
#include "Event.h"
#include "Epoller.h"
#include "EventLoop.h"
#include "TcpSource.h"

class TcpServer;

const int IO_BUFFER_SIZE = 8192;
class TcpConnection {

    TcpConnection(Socket socket, TcpSource *tcpSource, EventLoop *loop);

public:
    // lastReceiveTime: used by TimeWheelingPolicy, updated when inited or new message arrived
    uint32_t lastReceiveTime;

    enum State {
        BLANK, ESTABLISHED, CLOSED
    };

    State state() {
        return state_;
    }

    static TcpConnection make(int connfd, TcpSource *tcpSource, EventLoop *loop) {
        Socket connectedSokcet = Socket::makeConnected(connfd);
        connectedSokcet.setNonblock();
        return TcpConnection(std::move(connectedSokcet), tcpSource, loop);
    }

    static TcpConnection* makeHeapObject(int connfd, TcpSource *tcpSource, EventLoop *loop) {
        Socket connectedSokcet = Socket::makeConnected(connfd);
        connectedSokcet.setNonblock();
        return new TcpConnection(std::move(connectedSokcet), tcpSource, loop);
    }

    Buffer<IO_BUFFER_SIZE> &readBuffer() {
        return readBuffer_;
    }

    Buffer<IO_BUFFER_SIZE> &writeBuffer() {
        return writeBuffer_;
    }

    // 向writeBuffer_中填充数据并且调用send()来确保发送最终完成
    void send(const char *str, size_t len) {
        if (state_ != State::ESTABLISHED)
            Logger::fatal("connection closed, can't send data!\n");
        writeBuffer_.append(str, len);
        send();
    }

    // 确保非阻塞地及时发送writeBuffer_中所有值
    void send(bool isLast = false);

    Socket &socket() {
        return socket_;
    }

    EventLoop *eventLoop() {
        return loop_;
    }

    void destroy() {
        // todo 补充点什么
        state_ = CLOSED;
        loop_->epoller()->removeEvent(socket().fd());
    }

    void setDestoryCallback(std::function<void(TcpConnection*)> destoryCallback) {
        destoryCallback_ = destoryCallback;
    }

    void activeClose();

    TcpConnection(TcpConnection &&other) = default;

    // fixed 无法声明默认的析构函数，否则会引起TcpServer::readCallback()函数中的connections.insert插入错误！
    // 因为之前未声明移动构造函数，而显式声明一个（即使是默认的）析构函数会将移动构造函数定义为隐式删除的！
    ~TcpConnection() = default;

private:
    void sendNonblock();

private:
    Buffer<IO_BUFFER_SIZE> readBuffer_, writeBuffer_; // read和write是相对于用户而言
    Socket socket_;
    bool isWillClose_;
    std::function<void(TcpConnection*)>destoryCallback_ = nullptr; // only for FreeServerClient
    State state_;
    // 保存Tcpserver的引用是为了调用tcpServer_->closeConnection
    TcpSource *tcpSource_;
    // 保存EventLoop的引用是为了调用loop_->epoller()或指明谁在控制此连接
    EventLoop *loop_;
};


#endif //TESTLINUX_TCPCONNECTION_H
