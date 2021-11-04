//
// Created by Hello Peter on 2021/11/3.
//
#include "../src/HttpServer.h"

//extern std::atomic<uint64_t> sendWhenCloseCnts;
//extern std::atomic<uint64_t> totalConnectionsCnts;
//extern std::atomic<uint64_t> totalReadableCnts;

int main() {
    HttpServer server(InAddr("15558"));
    server.addServiceHandler("/port", [](TcpConnection *conn) {
        HttpResponse response;
        response.addBody("15558");
        std::string res = response.to_string();
        conn->send(res.c_str(), res.size());
    });
    server.addServiceHandler("/perf", [](TcpConnection *conn) {
        HttpResponse response;
        // todo
        std::string res = response.to_string();
        conn->send(res.c_str(), res.size());
    });

    // 开启多线程后性能下降好几倍？
    server.start(0);
}
