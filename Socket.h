//
// Created by Hello Peter on 2021/9/7.
//
#pragma once

#include <unistd.h>
#include <algorithm>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <cassert>

class InAddr;

// 表示一个listenfd或者connfd
class Socket {
private:
    Socket(int sockfd) : sockfd_(sockfd) {}

public:
    static Socket makeListened(int family = AF_INET, int type = SOCK_STREAM, int protocol = 0) {
        int sockfd = ::socket(family, type, protocol);
        if (sockfd == -1)
            assert(0);
        return Socket(sockfd);
    }

    static Socket makeConnected(int connfd) {
        return Socket(connfd);
    }

    Socket(Socket &&other) {
        sockfd_ = other.sockfd_;
        other.sockfd_ = 0;
    }

    ~Socket() {
        if (sockfd_ != 0)
            close(sockfd_);
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

    void shutdown(int rdwr);

    void setTcpNoDelay();

    void setReuseAddr();

    void setReusePort();

    void resetClose();

private:
    int sockfd_;
};
