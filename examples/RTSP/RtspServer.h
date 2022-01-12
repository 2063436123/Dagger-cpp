//
// Created by Hello Peter on 2021/11/3.
//

#ifndef TESTLINUX_HTTPSERVER_H
#define TESTLINUX_HTTPSERVER_H

#include <utility>
#include <functional>
#include "../../src/TcpServer.h"

enum STATE {
    NONE,
    PLAYING,
    PAUSED,
    DESTORYED
};

struct SessionState {
    SessionState(std::string id, std::string URI, std::string version) : cur_state(NONE) {
        this->id = std::move(id);
        this->URI = std::move(URI);
        this->version = std::move(version);
    }
    std::string id; // 8个数字或更长
    std::string URI;
    std::string version;
    STATE cur_state;
};

// for rtsp request parsing
struct RtspRequest {
    std::string method;
    std::string URI;
    std::string version;    // crlf
    std::unordered_map<std::string, std::string> headers;    // crlf crlf
    std::string body;    // crlf
};

struct RtspResponse {
    std::string version;
    std::string status_code;
    std::string status_cause;    // crlf
    std::unordered_map<std::string, std::string> headers;    // crlf crlf
    std::string body;    // crlf
};

class RtspServer {
public:
    explicit RtspServer(InAddr bindAddr) : codec_(Codec::UNLIMITED_MODEL, 0),
                                           server_(Socket::makeNewSocket(), bindAddr, &loop_, codec_) {
        server_.setConnMsgCallback([this](TcpConnection *conn) { handleRtspRequest(conn); });
    }

    void start(int workerNums = 0) {
        server_.start(workerNums);
    }

    ~RtspServer() {
        server_.stop();
    }

private:
    static void handleRtspRequest(TcpConnection *conn);

    EventLoop loop_;
    Codec codec_;
    TcpServer server_;

};

#endif //TESTLINUX_HTTPSERVER_H
