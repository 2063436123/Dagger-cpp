//
// Created by Hello Peter on 2021/10/22.
//

#include "../../src/TcpClient.h"
#include <iostream>

using namespace std;

std::atomic<uint64_t> bytes = 0;
int i = 0;

void whenMsgArrived(TcpConnection *connection) {
    auto &buf = connection->readBuffer();
    connection->send(buf.peek(), buf.readableBytes());
    bytes += buf.readableBytes();
    buf.retrieveAll();
}

const int N = 16 * 1024; // FIXME: N过大时，buffer报错out_of_range


void taskPerSecond() {
    ++i;
    uint64_t oldBytes = bytes.fetch_and(0);
    cout << oldBytes / 1024 << "K, " << i << "S, speed: " <<
         1.0 * oldBytes / 1024 / 1024 / 1 << "M/S\n";
}

int main() {
    EventLoop loop;
    TcpClient client(&loop, Codec(Codec::UNLIMITED_MODEL, 0));

    client.setConnMsgCallback(whenMsgArrived);
    client.addTimedTask(1, 1000, taskPerSecond);
    client.addWorkerThreads(3);

    std::string msg(N, ' ');

    for (int i = 0; i < 100; i++) {
        auto conn = client.connect(InAddr("12345", "127.0.0.1"));
        conn->send(msg.c_str(), msg.size());
    }
    client.join();
}

