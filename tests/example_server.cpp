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
    cout << "read: " << std::string(buf.peek(), buf.readableBytes());

    conn->send(buf.peek(), buf.readableBytes());

    buf.retrieveAll();
    // 如果不开启IDLE_CONNECTIONS_MANAGER，那么要么等待对端主动关闭（可能无限等待），要么在这里主动关闭。
    conn->activeClose();
}

void whenClose(TcpConnection *conn) {
    Socket &s = conn->socket();
//    cout << "conn terminated! from " << endl;
    // fixed 此时不该调用peerInAddr()，因为可能对端已经关闭了（当对端而非我端主动关闭时）
    // s.peerInAddr().ipPortStr() << " to " << s.localInAddr().ipPortStr() << endl;
}

int i = 0;
void taskPerSecond() {
    cout << to_string(i++) << "second" << endl;
}

int main() {
    if (Options::setMaxFiles(1048576) < 0)
        Logger::sys("getMaxFiles error");
    Options::getMaxFiles();

    auto s = Socket::makeNewSocket();
    EventLoop loop;
    TcpServer server(std::move(s), InAddr("12345"), &loop);

    // 添加定时任务
//    server.addTimedTask(1000, 2000, taskPerSecond);

    server.setConnEstaCallback(whenNewConnectionEstablished);
    server.setConnMsgCallback(whenMsgArrived);
    server.setConnCloseCallback(whenClose);
//    server.start();
    server.start(4);
}