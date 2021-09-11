//
// Created by Hello Peter on 2021/9/11.
//

#ifndef TESTLINUX_TCPCONNECTION_H
#define TESTLINUX_TCPCONNECTION_H

#include "Buffer.h"
#include "Event.h"
#include "Epoller.h"

class TcpConnection {
    enum State {
        BLANK, ESTABLISHED, CLOSED
    };

    TcpConnection(Socket socket, Epoller *epoller) : socket_(std::move(socket)), state_(BLANK), epoller_(epoller) {}

public:
    static TcpConnection make(int connfd, Epoller *epoller) {
        return TcpConnection(Socket::makeConnected(connfd), epoller);
    }

    Buffer<8192> &readBuffer() {
        return readBuffer_;
    }

    Buffer<8192> &writeBuffer() {
        return writeBuffer_;
    }

    // 像writeBuffer_中填充值并且调用send()来确保发送最终完成
    void send(const char* str, size_t len) {
        writeBuffer_.append(str, len);
        send();
    }

    // 确保非阻塞地及时发送writeBuffer_中所有值
    void send() {
        assert(writeBuffer_.readableBytes() > 0);
        std::shared_ptr<Event> event = epoller_->getEvent(socket_.fd());
        event->setWritable(true);
        event->setWriteCallback(std::bind(&TcpConnection::sendNonblock, this));
    }

    Socket &socket() {
        return socket_;
    }

    void destroy() {
        // todo
        state_ = CLOSED;
    }

    // fixme 无法声明默认的析构函数，否则会引起TcpServer::readCallback()函数中的connections.insert插入错误！
    // ~TcpConnection() = default;
private:
    void sendNonblock() {
        // user -> buffer(writable)
        // buffer(readable) -> socket
        // todo send nonblockly!
        if (writeBuffer_.readableBytes() == 0) {
            auto event = epoller_->getEvent(socket().fd());
            event->setWritable(false);
            return;
        }
        int n = ::write(socket().fd(), writeBuffer_.peek(),
                        writeBuffer_.readableBytes());
        if (n == -1 && errno != EAGAIN)
            assert(0);
        writeBuffer_.retrieve(n);
    }
private:
    Buffer<8192> readBuffer_, writeBuffer_;
    Socket socket_;
    State state_;
    Epoller *epoller_;
};


#endif //TESTLINUX_TCPCONNECTION_H
