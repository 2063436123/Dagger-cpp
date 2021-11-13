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

int N = 4 * 1024;

void taskPerSecond() {
    ++i;
    uint64_t oldBytes = bytes.fetch_and(0);
    cout << oldBytes / 1024 << "K, " << i << "S, speed: " <<
         1.0 * oldBytes / 1024 / 1024 / 1 << "M/S\n";
}

// ./a.out threadNums connectionNums blockSize
int main(int argc, char **argv) {
    EventLoop loop;
    TcpClient client(&loop, Codec(Codec::UNLIMITED_MODEL, 0));

    client.setConnMsgCallback(whenMsgArrived);
    client.addTimedTask(1, 1000, taskPerSecond);
    if (argc > 1)
        client.addWorkerThreads(atoi(argv[1]));
    else
        client.addWorkerThreads(3);

    std::string msg(N, ' ');

    int connectionNums = (argc > 2) ? atoi(argv[2]) : 1;
    for (int i = 0; i < connectionNums; i++) {
        auto conn = client.connect(InAddr("12345", "127.0.0.1"));
        conn->send(msg.c_str(), msg.size());
    }
    client.join();
}

