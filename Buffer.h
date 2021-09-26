//
// Created by Hello Peter on 2021/9/4.
//

#pragma once

#include <vector>
#include <iostream>
#include <cstring>
#include <cassert>
#include <algorithm>
#include <unistd.h>
#include "Socket.h"

/**
 * / discarded / readable / writable /
 * */
template<size_t N = 8192>
class Buffer {
public:
    Buffer() : vec_(N), readIndex_(0), writeIndex_(0) {}

    size_t readableBytes() const {
        return writeIndex_ - readIndex_;
    }

    size_t writableBytes() const {
        return vec_.size() - writeIndex_;
    }

    size_t discardableBytes() const {
        return readIndex_;
    }

    size_t totalSize() const {
        return vec_.size();
    }

    // 当socket可读时，socket->buffer(writable)
    ssize_t readFromSocket(Socket &s, size_t maxBytes = 4096) {
        int connfd = s.fd();
        if (writableBytes() < maxBytes) {
            if (discardableBytes() >= maxBytes) {
                // 复用discarded空间
            } else {
                // 不得不分配新空间
                size_t newSize = std::max(vec_.size() * 2, maxBytes + readableBytes());
                vec_.resize(newSize);
            }
            memcpy(&vec_[0], &vec_[readIndex_], readableBytes());
            writeIndex_ = readableBytes();
            readIndex_ = 0;
        }
        assert(writableBytes() >= maxBytes);
        ssize_t n = ::read(connfd, &vec_[writeIndex_], maxBytes);
        assert(n >= 0 && n <= maxBytes);
        writeIndex_ += n;
        return n;
    }

    // 当socket可写时，buffer(readable)->socket
    ssize_t writeToSocket(Socket &s, size_t maxBytes = 4096) {
        int connfd = s.fd();
        // clog << "test1: " << std::min(readableBytes(), maxBytes) << "&" << std::string(peek(), std::min(readableBytes(), maxBytes));
        int n = ::write(connfd, peek(), std::min(readableBytes(), maxBytes));
        assert(n >= 0 && n <= maxBytes);
        readIndex_ += n;
        return n;
    }

    void append(const char *s, size_t len) {
        if (writableBytes() < len) {
            if (discardableBytes() >= len) {
                // 复用discarded空间
            } else {
                // 不得不分配新空间
                size_t newSize = std::max(vec_.size() * 2, len + readableBytes());
                vec_.resize(newSize);
            }
            memcpy(&vec_[0], &vec_[readIndex_], readableBytes());
            writeIndex_ = readableBytes();
            readIndex_ = 0;
        }
        memcpy(&vec_[writeIndex_], s, len);
        writeIndex_ += len;
        assert(writeIndex_ <= totalSize());
    }

    const char *peek() const {
        return &vec_[readIndex_];
    }

    const char *retrieve(size_t len) {
        const char *ret = &vec_[readIndex_];
        if (readIndex_ + len > writeIndex_)
            return nullptr;
        readIndex_ += len;
        return ret;
    }

    const char *retrieveAll() {
        return retrieve(readableBytes());
    }

    const char *findStr(const char *str) const {
        // 想用strstr，但是无法指定长度，故使用memmem
        return (const char *) memmem(peek(), readableBytes(), str, strlen(str));
    }

private:
    std::vector<char> vec_;
    size_t readIndex_;
    size_t writeIndex_;
};

typedef Buffer<512> SBuffer;
typedef Buffer<4096> NBuffer;
typedef Buffer<65536> LBuffer;

//int main() {
//    SBuffer buffer1;
//    size_t appendSize;
//    while (cin >> appendSize) {
//        char *buf = new char[appendSize];
//        buffer1.append(buf, appendSize);
//        cout << "totalSize = " << buffer1.totalSize() << ", readableBytes = " << buffer1.readableBytes() <<
//            ", writableBytes = " << buffer1.writableBytes() << endl;
//    }
//}
