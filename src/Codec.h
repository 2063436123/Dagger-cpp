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
        HTTP_MODEL, LENGTH_DATA_MODEL, FIXED_LENGTH_MODEL, UNLIMITED_MODEL
    };

    explicit Codec(std::function<bool(TcpConnection *)> cb) : cb_(std::move(cb)) {}

    Codec(MODEL model, DataType data) {
        if (model == HTTP_MODEL) {
            cb_ = [data](TcpConnection *conn) { return http_request_default_callback(conn, data); };
        } else if (model == LENGTH_DATA_MODEL) {
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
    static bool length_data_default_callback(TcpConnection *conn, DataType length_self_size) noexcept {
        DataType length_size = length_self_size;
        auto &read_buffer = conn->readBuffer();
        if (read_buffer.readableBytes() < length_size)
            return false;
        unsigned long long data_len = 0;
        try {
            DataType handle_size;
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
    static bool fixed_length_default_callback(TcpConnection *conn, DataType requiredBytes) {
        return conn->readBuffer().readableBytes() > requiredBytes;
    }

    // 当readBuffer中有一个完整的http request报文时，返回true
    static bool http_request_default_callback(TcpConnection *conn, DataType flags) {
        auto& buf = conn->readBuffer();
        // 暂时不考虑content-length及request body
        const char* pc = buf.findStr("\r\n\r\n");
        if (!pc)
            return false;
        *(const_cast<char*>(pc + 3)) = '\0';
        return true;
    }

private:
    std::function<bool(TcpConnection *)> cb_;
};

#endif //TESTLINUX_CODEC_H

#pragma clang diagnostic pop