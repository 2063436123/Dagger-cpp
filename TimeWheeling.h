//
// Created by Hello Peter on 2021/10/20.
//

#ifndef TESTLINUX_TIMEWHEELING_H
#define TESTLINUX_TIMEWHEELING_H

#include <vector>
#include <deque>
#include <cassert>

extern uint32_t timeInProcess;

using TimerWheeling = std::vector<std::deque<TcpConnection *>>;
class TimerWheelingPolicy {
    const int MAX_IDLE_SECONDS = 60 * 10; // 10 minutes
public:
    TimerWheelingPolicy() : wheeling_(MAX_IDLE_SECONDS) {
    }

    void eraseIdleConnections() {
        assert(MAX_IDLE_SECONDS == wheeling_.size());
        int checkIndex = (curBucketIndex_ + MAX_IDLE_SECONDS - 1) % MAX_IDLE_SECONDS;
        // 可能的性能瓶颈，因为每一秒都要O(n)遍历并检查所有的已建立的连接是否过期
        // todo
        for (TcpConnection* conn : wheeling_[checkIndex]) {
            if (conn->state() == TcpConnection::State::CLOSED) {
                delete conn;
            } else if (conn->state() == TcpConnection::State::ESTABLISHED) {
                // 查看是否空闲
                int now = timeInProcess;
                if (now - conn->lastReceiveTime > MAX_IDLE_SECONDS) {
                    // 1. 空闲
                    Logger::info("conn destroy, connfd = {}, lastReceiveTime = {}, now = {}\n",
                                 conn->socket().fd(), conn->lastReceiveTime, timeInProcess);
                    conn->destroy();
                    // todo do what after destroying
                    delete conn;
                }
                else {
                    // 2. 不空闲
                    addNewConnection(conn);
                }
            } else {
                Logger::info("todo BLANK等状态的处理.");
            }
        }
        wheeling_[checkIndex].clear();
        timeElasped();
    }

    void addNewConnection(TcpConnection* conn) {
        wheeling_[curBucketIndex_].push_back(conn);
    }

private:
    // be called by a timer
    void timeElasped() {
        curBucketIndex_ = (curBucketIndex_ + 1) % MAX_IDLE_SECONDS;
    }

    TimerWheeling wheeling_; // 维护所有的已建立的连接，被wheelPolicy_管理
    int curBucketIndex_ = 0;
};

#endif //TESTLINUX_TIMEWHEELING_H
