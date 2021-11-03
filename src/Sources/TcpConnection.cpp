//
// Created by Hello Peter on 2021/9/11.
//

#include "../TcpConnection.h"

#include <utility>
#include "../TcpServer.h"


TcpConnection::TcpConnection(Event *event, TcpSource *tcpSource, EventLoop *loop) : event_(event),
                                                                                    socket_(Socket::makeConnected(
                                                                                            event_->fd())),
                                                                                    state_(ESTABLISHED),
                                                                                    isWillClose_(false),
                                                                                    tcpSource_(tcpSource),
                                                                                    loop_(loop) {
    socket_.setNonblock();
}

void TcpConnection::sendNonblock() {
    // user -> buffer(writable)
    // buffer(readable) -> socket
    if (writeBuffer_.readableBytes() == 0) {
        event_->setWritable(false);
        if (isWillClose_) {
            isWillClose_ = false;
            activeClose();
        }
        return;
    }
    ssize_t n = ::write(socket().fd(), writeBuffer_.peek(),
                        writeBuffer_.readableBytes());
    if (n == -1 && errno != EAGAIN)
        Logger::sys("nonblock write error");
    writeBuffer_.retrieve(n);
}

void TcpConnection::send(bool isLast) {
    assert(writeBuffer_.readableBytes() > 0);
    isWillClose_ = isLast;
    event_->setWritable(true);
    event_->setWriteCallback([this] { sendNonblock(); });
}

void TcpConnection::activeClose() {
    tcpSource_->closeConnection(this, destoryCallback_);
}


