//
// Created by Hello Peter on 2021/9/11.
//

#include "../TcpConnection.h"

#include <utility>
#include "../TcpServer.h"


TcpConnection::TcpConnection(Socket socket, TcpSource *tcpSource, EventLoop *loop) : socket_(std::move(socket)),
                                                                                     state_(ESTABLISHED), isWillClose(false),
                                                                                     tcpSource_(tcpSource),
                                                                                     loop_(loop) {}

void TcpConnection::sendNonblock() {
    // user -> buffer(writable)
    // buffer(readable) -> socket
    if (writeBuffer_.readableBytes() == 0) {
        auto event = loop_->epoller()->getEvent(socket().fd());
        event->setWritable(false);
        if (isWillClose) {
            isWillClose = false;
            activeClose();
        }
        return;
    }
    int n = ::write(socket().fd(), writeBuffer_.peek(),
                    writeBuffer_.readableBytes());
    if (n == -1 && errno != EAGAIN)
        Logger::sys("nonblock write error");
    writeBuffer_.retrieve(n);
}

void TcpConnection::send(bool isLast) {
    assert(writeBuffer_.readableBytes() > 0);
    isWillClose = isLast;
    std::shared_ptr<Event> event = loop_->epoller()->getEvent(socket_.fd());
    event->setWritable(true);
    event->setWriteCallback(std::bind(&TcpConnection::sendNonblock, this));
}

void TcpConnection::activeClose() {
    tcpSource_->closeConnection(this, destoryCallback_);
}


