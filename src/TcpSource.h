//
// Created by Hello Peter on 2021/10/22.
//

#ifndef TESTLINUX_TCPSOURCE_H
#define TESTLINUX_TCPSOURCE_H
class TcpConnection;

// for TcpConnection contains TcpServer/TcpClient
class TcpSource {
public:
    // @param destoryCallback 只服务于FreeServerClient，而Server和Client并不适用此参数
    virtual void closeConnection(TcpConnection *connection, std::function<void(TcpConnection*)> destoryCallback) = 0;
};
#endif //TESTLINUX_TCPSOURCE_H
