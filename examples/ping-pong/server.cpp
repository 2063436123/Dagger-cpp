//
// Created by Hello Peter on 2021/9/7.
//

#include "../../src/TcpServer.h"

using namespace std;

std::atomic<uint64_t> bytes = 0;

void whenMsgArrived(TcpConnection *conn) {
    auto &buf = conn->readBuffer();
//    bytes += buf.readableBytes();
    conn->send(buf.peek(), buf.readableBytes());

    buf.retrieveAll();
}

int i = 0;

void taskPerSecond() {
    ++i;
    uint64_t oldBytes = bytes.fetch_and(0);
    cout << oldBytes / 1024 << "K, " << i << "S, speed: " << 1.0 * oldBytes / 1024 / 1024 / 1 << "M/S\n";
}

int main() {
    if (Options::setMaxFiles(1048576) < 0)
        Logger::sys("getMaxFiles error");
    Options::getMaxFiles();
//    Options::setTcpRmem(1024);
//    Options::setTcpWmem(1024);

    auto s = Socket::makeNewSocket();
    EventLoop loop;
    TcpServer server(std::move(s), InAddr("12345"), &loop,
                     Codec(Codec::UNLIMITED_MODEL, 0));

    server.setConnMsgCallback(whenMsgArrived);

//    server.addTimedTask(1000, 1000, taskPerSecond);
    server.start(3);
}