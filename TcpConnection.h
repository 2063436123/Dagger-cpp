//
// Created by Hello Peter on 2021/9/11.
//

#ifndef TESTLINUX_TCPCONNECTION_H
#define TESTLINUX_TCPCONNECTION_H

#include "Buffer.h"
#include "Event.h"
#include "Epoller.h"
#include "EventLoop.h"

class TcpServer;

class TcpConnection {
    enum State {
        BLANK, ESTABLISHED, CLOSED
    };

    TcpConnection(Socket socket, TcpServer *tcpServer, EventLoop *loop);

public:
    static TcpConnection make(int connfd, TcpServer *tcpServer, EventLoop *loop) {
        return TcpConnection(Socket::makeConnected(connfd), tcpServer, loop);
    }

    Buffer<8192> &readBuffer() {
        return readBuffer_;
    }

    Buffer<8192> &writeBuffer() {
        return writeBuffer_;
    }

    // 像writeBuffer_中填充值并且调用send()来确保发送最终完成
    void send(const char *str, size_t len) {
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
        // todo
        state_ = CLOSED;
    }

    void activeClose();

    TcpConnection(TcpConnection &&other) = default;

    // fixed 无法声明默认的析构函数，否则会引起TcpServer::readCallback()函数中的connections.insert插入错误！
    // 因为之前未声明移动构造函数，而显式声明一个（即使是默认的）析构函数会将移动构造函数定义为隐式删除的！
    ~TcpConnection() = default;

private:
    void sendNonblock();

private:
    Buffer<8192> readBuffer_, writeBuffer_;
    Socket socket_;
    bool isWillClose;
    State state_;
    // 保存Tcpserver的引用是为了调用tcpServer_->closeConnection
    TcpServer *tcpServer_;
    // 保存EventLoop的引用是为了调用loop_->epoller()或指明谁在控制此连接
    EventLoop *loop_;
};


#endif //TESTLINUX_TCPCONNECTION_H
