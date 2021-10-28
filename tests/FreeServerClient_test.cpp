//
// Created by Hello Peter on 2021/10/27.
//

#include "../src/FreeServerClient.h"

void whenConnEsta(TcpConnection* conn) {
    Logger::info("new conn{}\n", conn->socket().fd());
}

void whenMsgArrived(TcpConnection* conn) {
    Logger::info("new msg:{}\n", std::string_view(conn->readBuffer().peek(), conn->readBuffer().readableBytes()));

}

void whenConnClose(TcpConnection* conn) {
    Logger::info("conn{} closed.\n", conn->socket().fd());
}

int main() {
    EventLoop loop;
    FreeServerClient serverClient(&loop);
    serverClient.createServerPort(InAddr("12344"), whenConnEsta, whenMsgArrived, whenConnClose, nullptr);
    auto conn = serverClient.createClientConn(InAddr("12345"), whenConnEsta, whenMsgArrived, whenConnClose, nullptr);
    conn->send("hello", 5);
    serverClient.start();
}