//
// Created by Hello Peter on 2021/11/19.
//
#include "../src/TcpServer.h"
#include <iostream>

using namespace std;

int i = 0;

void func() {
    cout << "in func: " << i++ << endl;
}

TcpServer *server;
TimerHandler *handler1;

void func2() {
    cout << "in func2" << endl;
    server->cancelTimer(*handler1);
}

int main() {
    EventLoop loop;
    TcpServer server(Socket::makeNewSocket(), InAddr("0.0.0.0:10000"), &loop, Codec());

    // 添加定时任务
    auto handler = server.addTimedTask(0, 1000, func);
    ::server = &server;
    handler1 = &handler;
    server.addOneTask(2000, func2);

    server.start();
}
