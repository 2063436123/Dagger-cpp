//
// Created by Hello Peter on 2021/9/7.
//
#pragma once

#include <unistd.h>
#include <algorithm>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <cassert>
#include "Options.h"

class InAddr;

// 表示一个listenfd或者connfd
class Socket {
private:
    Socket(int sockfd) : sockfd_(sockfd) {}

public:
    static Socket makeNewSocket(int family = AF_INET, int type = SOCK_STREAM, int protocol = 0) {
        int sockfd = ::socket(family, type, protocol);
        if (sockfd == -1)
            Logger::sys("socket error");
        return Socket(sockfd);
    }

    static Socket makeConnected(int connfd) {
        return Socket(connfd);
    }

    Socket(const Socket&) = delete;

    Socket(Socket &&other)  noexcept {
        sockfd_ = other.sockfd_;
        other.sockfd_ = 0;
    }

    ~Socket() {
        if (sockfd_ != 0)
            ::close(sockfd_);
    }

    int fd() const {
        return sockfd_;
    }

    InAddr localInAddr();

    InAddr peerInAddr();

    void bindAddr(const InAddr &localAddr);

    void listen(int backlog);

    int accept(InAddr &peerAddr);

    int accept();

    void connect(const InAddr& peerAddr);

    void setNonblock();

    void shutdown(int rdwr);

    void setTcpNoDelay();

    void setReuseAddr();

    void setReusePort();

    void setQuickAck();

    void resetClose();

    bool checkHasError();

private:
    int sockfd_;
};
