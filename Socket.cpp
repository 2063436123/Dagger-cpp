//
// Created by Hello Peter on 2021/9/7.
//
#include "Socket.h"
#include "InAddr.h"
#include "Buffer.h"

using namespace std;

InAddr Socket::localInAddr() {
    sockaddr sock;
    // fixed : len必须赋初值为sizeof(sockaddr)，因为这是一个值-结果参数而非结果参数！
    socklen_t len = sizeof(sock);
    if (::getsockname(sockfd_, &sock, &len) < 0)
        assert(0);
    struct sockaddr_in *ipv4Addr = (sockaddr_in *) &sock;
    return InAddr(ipv4Addr->sin_port, ipv4Addr->sin_addr, ipv4Addr->sin_family);
}

InAddr Socket::peerInAddr() {
    sockaddr sock;
    socklen_t len = sizeof(sock);
    if (::getpeername(sockfd_, &sock, &len) < 0) {
        cout << strerror(errno) << endl;
        assert(0);
    }
    struct sockaddr_in *ipv4Addr = (sockaddr_in *) &sock;
    return InAddr(ipv4Addr->sin_port, ipv4Addr->sin_addr, ipv4Addr->sin_family);
}

void Socket::bindAddr(const InAddr &localAddr) {
    if (::bind(sockfd_, localAddr.sockAddr(), sizeof(localAddr)) < 0) {
        cout << strerror(errno) << endl;
        cout << "sockfd_: " << sockfd_ << endl;
        assert(0);
    }
}

void Socket::listen(int backlog) {
    if (::listen(sockfd_, backlog) < 0)
        assert(0);
}

int Socket::accept(InAddr &peerAddr) {
    sockaddr sock;
    socklen_t len;
    int connfd_ = ::accept(sockfd_, &sock, &len);
    if (connfd_ < 0)
        assert(0);
    // todo set connfd nonblock
    struct sockaddr_in *ipv4Addr = (sockaddr_in *) &sock;
    peerAddr = InAddr(ipv4Addr->sin_port, ipv4Addr->sin_addr, ipv4Addr->sin_family);
    return connfd_;
}

int Socket::accept() {
    int connfd_ = ::accept(sockfd_, nullptr, nullptr);
    if (connfd_ < 0)
        assert(0);
    return connfd_;
}

void Socket::shutdown(int rdwr = SHUT_WR) {
    if (::shutdown(sockfd_, rdwr) < 0)
        assert(0);
}

void Socket::setTcpNoDelay() {
    int on = 1;
    if (setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on)) < 0)
        assert(0);
}

void Socket::setReuseAddr() {
    int on = 1;
    if (setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
        assert(0);
}

void Socket::setReusePort() {
    int on = 1;
    if (setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on)) < 0)
        assert(0);
}

void Socket::resetClose() {
    linger lr{.l_onoff = 1, .l_linger = 0};
    if (setsockopt(sockfd_, SOL_SOCKET, SO_LINGER, &lr, sizeof(lr)) < 0)
        assert(0);
    close(sockfd_);
    sockfd_ = 0;
}

//int main() {
//    Socket socket1 = Socket::make();
//    socket1.setReuseAddr();
//    socket1.bindAddr(InAddr("12345", "0.0.0.0", AF_INET));
//    socket1.listen(100);
//
//    Socket socket2 = Socket::makeConnected(socket1.accept());
//
//    cout << socket2.localInAddr().ipPortStr() << " --- "
//         << socket2.peerInAddr().ipPortStr() << endl;
//
//    while (true) {
//        cout << "new while begin" << endl;
//        Buffer buffer1;
//        bool exit = false;
//        int n;
//        while (!exit && (n = buffer1.readFromSocket(socket2)) > 0) {
//            cout << "received: $" << std::string(buffer1.peek(), buffer1.readableBytes())
//                 << '$' << endl;
//            if (buffer1.findStr("\r\n\r\n") != nullptr) {
//                cout << "find Str!" << endl;
//                exit = true;
//            }
//            buffer1.retrieveAll();
//        }
//
//        if (n == 0) {
//            cout << "eof" << endl;
//            break;
//        }
//
//        char buf[4000];
//        n = sprintf(buf, "HTTP/1.1 200 OK\r\nContent-Type:text/html\r\nContent-length:10\r\n\r\n1234567890123\r\n");
//        Buffer buffer2;
//        buffer2.append(buf, n);
//        buffer2.writeToSocket(socket2);
//
//        socket2.shutdown();
//    }
//}
