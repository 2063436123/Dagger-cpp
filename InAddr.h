//
// Created by Hello Peter on 2021/9/6.
//

#pragma once
#include <vector>
#include <iostream>
#include <cstring>
#include <cassert>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class InAddr {
public:
    InAddr(const std::string &sin_port, const std::string &sin_addr = "0.0.0.0", sa_family_t sa_family = AF_INET)
            : InAddr(sin_port.c_str(), sin_addr.c_str(), sa_family) {}

    InAddr(const char *sin_port, const char *sin_addr = "0.0.0.0", sa_family_t sa_family = AF_INET) {
        in_addr addr;
        in_port_t portNum = atoi(sin_port);
        assert(portNum != 0); // 说明sin_port不是有效的数字 或者 就是0，但0也不应该被设置为端口.

        inet_pton(sa_family, sin_addr, &addr);
        in_port_t port = htons(portNum);

        initAddr(port, addr, sa_family);
    }

    // 注意，此构造函数的sin_port和sin_addr要求是主机字节序
    InAddr(in_port_t sin_port, in_addr sin_addr = in_addr{.s_addr = INADDR_ANY},
           sa_family_t sa_family = AF_INET) {
        initAddr(sin_port, sin_addr, sa_family);
    }


    std::string ipPortStr() const {
        char buf[INET_ADDRSTRLEN + 10];
        inet_ntop(addr_.sa_family, &((sockaddr_in *) &addr_)->sin_addr, buf, INET_ADDRSTRLEN);
        in_port_t port = ntohs(((sockaddr_in *) &addr_)->sin_port);
        return std::string(buf) + ':' + std::to_string(port);
    }

    const sockaddr *sockAddr() const {
        return &addr_;
    }

private:
    void initAddr(in_port_t sin_port, in_addr sin_addr, sa_family_t sa_family) {
        bzero(&addr_, sizeof(addr_));
        addr_.sa_family = sa_family;
        if (sa_family == AF_INET) {
            sockaddr_in *ipv4Addr = (sockaddr_in *) &addr_;
            ipv4Addr->sin_port = sin_port;
            ipv4Addr->sin_addr = sin_addr;
        } else if (sa_family == AF_INET6) {
            // todo，sockaddr类型的addr_不足以承载ipv6的sockaddr_in6，除非使用sockaddr_storage
        } else {
            std::cout << "sa_family: " << sa_family << std::endl;
            assert(0);
        }
    }

private:
    struct sockaddr addr_;
};

//int main() {
//    InAddr addr1("30", "127.0.0.1", AF_INET);
//    cout << addr1.ipPortStr() << endl;
//
//    auto ret = addr1.sockaddr();
//    ret.sin_port = 0x00ff;
//    InAddr addr2(ret.sin_port);
//    cout << addr2.ipPortStr() << endl;
//}