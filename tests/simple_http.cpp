//
// Created by Hello Peter on 2021/11/3.
//
#include "../src/HttpServer.h"
//

uint64_t newConnRequests;
uint64_t reuseRequests;
uint64_t runtimeConnNums;
uint64_t recoveryConnNums;

int main() {
    ObjectPool::addObjectCache<TcpConnection>(10000);
    ObjectPool::addObjectCache<Event>(10000);

    HttpServer server(InAddr("0.0.0.0:15558"));
    server.addServiceHandler("/port", [](TcpConnection *conn) {
        HttpResponse response;
        response.addBody("15558");
        std::string res = response.to_string();
        conn->send(res.c_str(), res.size());
    });
    server.addServiceHandler("/perf", [](TcpConnection *conn) {
        HttpResponse response;
        std::string body(fmt::format("{} {} {} {}<br>", newConnRequests, reuseRequests, runtimeConnNums, recoveryConnNums));
        body += fmt::format("connsPerThread: {} {} {} {}<br>", connsPerThread[0], connsPerThread[1], connsPerThread[2], connsPerThread[3]);
        response.addBody(body);
        std::string res = response.to_string();
        conn->send(res.c_str(), res.size());
    });


    // 开启多线程后性能下降好几倍？
    server.start(3);
}
