//
// Created by Hello Peter on 2021/9/7.
//

#include "../src/TcpServer.h"

using namespace std;

void whenNewConnectionEstablished(TcpConnection *conn) {
    Socket &s = conn->socket();
//    cout << "conn connected! from " << s.peerInAddr().ipPortStr() << " to " << s.localInAddr().ipPortStr() << " ";
//    cout << "fd: " << s.fd() << endl;
}

void whenMsgArrived(TcpConnection *conn) {
    Socket &s = conn->socket();
    // todo exit safely when bye

    auto &buf = conn->readBuffer();
//    conn->send(buf.peek(), buf.readableBytes());
    std::string response_str("HTTP/1.1 200 OK\r\n"
                             "content-type: text/html; charset=utf-8\r\n"
                             "content-length: 49\r\n"
                             "connection: close\r\n"
                             "\r\n"
                             "<title>The Service haven't been provided.</title>\r\n");
    buf.retrieveAll();
    conn->send(response_str.c_str(), response_str.size());
    // 如果不开启IDLE_CONNECTIONS_MANAGER，那么要么等待对端主动关闭（可能无限等待），要么在这里主动关闭。
    //conn->activeClose();
//    conn->socket().resetClose();
}

void whenClose(TcpConnection *conn) {
    Socket &s = conn->socket();
    //Logger::info("conn terminated! fd = {}\n", s.fd());
    // fixed 此时不该调用peerInAddr()，因为可能对端已经关闭了（当对端而非我端主动关闭时）
    // s.peerInAddr().ipPortStr() << " to " << s.localInAddr().ipPortStr() << endl;
}

int i = 0;

void taskPerSecond() {
    ++i;
}

int main() {
//    Options::setTcpRmem(1024);
//    Options::setTcpWmem(1024);

    auto s = Socket::makeNewSocket();
    EventLoop loop;
    TcpServer server(std::move(s), InAddr("12345", "0.0.0.0"), &loop,
                     Codec(Codec::UNLIMITED_MODEL, 0));

    // 添加事件回调
    server.setConnEstaCallback(whenNewConnectionEstablished);
    server.setConnMsgCallback(whenMsgArrived);
    server.setConnCloseCallback(whenClose);

    // 添加定时任务
//    server.addTimedTask(1000, 1000, taskPerSecond);
    server.start(3);
}