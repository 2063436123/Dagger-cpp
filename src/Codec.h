//
// Created by Hello Peter on 2021/10/29.
//

#ifndef TESTLINUX_CODEC_H
#define TESTLINUX_CODEC_H

#include <utility>

#include "TcpConnection.h"

// 目前Codec只服务于接收方，发送方需要自己组织TCP负载的内容
class Codec {
public:
    enum /*class*/ MODEL {
        LENGTH_DATA_MODEL, FIXED_LENGTH_MODEL, UNLIMITED_MODEL
    };

    explicit Codec(std::function<bool(TcpConnection *)> cb) : cb_(std::move(cb)) {}

    Codec(MODEL model, size_t data) {
        if (model == LENGTH_DATA_MODEL) {
            cb_ = [data](TcpConnection *conn) { return length_data_default_callback(conn, data); };
        } else if (model == FIXED_LENGTH_MODEL) {
            cb_ = [data](TcpConnection *conn) { return fixed_length_default_callback(conn, data); };
        } else if (model == UNLIMITED_MODEL) {
            cb_ = [](TcpConnection *conn) { return true; };
        } else
            assert(0);
    }

    bool check(TcpConnection *connection) const {
        return cb_(connection);
    }

private:
    // 当readBuffer中已有 length(长度为length_size字节) + data 时，retrieve length部分，返回true
    static bool length_data_default_callback(TcpConnection *conn, size_t length_size) noexcept {
        auto &read_buffer = conn->readBuffer();
        if (read_buffer.readableBytes() < length_size)
            return false;
        unsigned long long data_len = 0;
        try {
            size_t handle_size;
            // todo 考虑手写简单的整数的stoull，以减少性能开销
            data_len = std::stoull(std::string(read_buffer.peek(), length_size), &handle_size);
            if (handle_size != length_size)
                Logger::info("stoull error.\n");
        } catch (...) {
            Logger::info("stoull exception caught.\n");
        }
        if (read_buffer.readableBytes() - length_size < data_len)
            return false;
        return true;
    }

    // 当readBuffer中有size个可读字节时，返回true
    static bool fixed_length_default_callback(TcpConnection *conn, size_t requiredBytes) {
        return conn->readBuffer().readableBytes() > requiredBytes;
    }


private:
    std::function<bool(TcpConnection *)> cb_;
};

#endif //TESTLINUX_CODEC_H
