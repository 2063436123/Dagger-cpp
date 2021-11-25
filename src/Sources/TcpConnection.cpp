//
// Created by Hello Peter on 2021/9/11.
//

#include "../TcpConnection.h"
#include "../TcpServer.h"


TcpConnection::TcpConnection(Event *event, TcpSource *tcpSource, EventLoop *loop) : data_(nullptr), event_(event),
                                                                                    socket_(Socket::makeConnected(
                                                                                            event_->fd())),
                                                                                    state_(ESTABLISHED),
                                                                                    isWillClose_(false),
                                                                                    tcpSource_(tcpSource),
                                                                                    loop_(loop) {
    socket_.setNonblock();
    //socket_.setQuickAck();
    //socket_.setTcpNoDelay();
}

void TcpConnection::sendNonblock() {
    // user -> buffer(writable)
    // buffer(readable) -> socket
    ssize_t n = ::write(socket().fd(), writeBuffer_.peek(),
                        writeBuffer_.readableBytes());
    if (n == -1 && errno != EAGAIN)
        Logger::sys("nonblock write error");
    writeBuffer_.retrieve(n);
    if (writeBuffer_.readableBytes() == 0) {
        event_->setWritable(false);
        if (isWillClose_) {
            activeClose();
        }
    }
}

void TcpConnection::send(bool isLast) {
    if (state() == BLANK)
        sleep(2); // for client: 等待连接建立, todo 使用cv
    assert(writeBuffer_.readableBytes() > 0);
    isWillClose_ = isLast;
    if (event_->isWritable()) // fixed 竞态条件：因为异步connect注册了一个可写回调来检查connect是否成功，那么在connect结果返回前将不能调用send.
        return;
    event_->setWritable(true);
    event_->setWriteCallback([this] { sendNonblock(); });
}

void TcpConnection::activeClose() {
    tcpSource_->closeConnection(this, destoryCallback_);
}


