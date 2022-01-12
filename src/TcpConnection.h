//
// Created by Hello Peter on 2021/9/11.
//

#ifndef TESTLINUX_TCPCONNECTION_H
#define TESTLINUX_TCPCONNECTION_H

#include <utility>

#include "Buffer.h"
#include "Event.h"
#include "Timer.h"
#include "Epoller.h"
#include "EventLoop.h"
#include "TcpSource.h"
//#include "ObjectPool.hpp"

class TcpServer;

const int IO_BUFFER_SIZE = 8192;
class TcpConnection {
    template<typename T, typename... Args>
    friend T* ObjectPool::getNewObject(Args &&...);
    TcpConnection(Event *event, TcpSource *tcpSource, EventLoop *loop);

public:
    // lastReceiveTime: used by TimeWheelingPolicy, updated when inited or new message arrived
    uint32_t lastReceiveTime{};

    enum State {
        BLANK, ESTABLISHED, WILL_CLOSED, CLOSED
    };

    State state() {
        return state_;
    }

    void setState(State state) {
        state_ = state;
    }

    static TcpConnection make(Event *event, TcpSource *tcpSource, EventLoop *loop) {
        return TcpConnection(event, tcpSource, loop);
    }

    static TcpConnection* makeHeapObject(Event *event, TcpSource *tcpSource, EventLoop *loop) {
        // todo 从连接池拿取新连接，除非连接池已空才使用new
        return ObjectPool::getNewObject<TcpConnection>(event, tcpSource, loop);
        //return new TcpConnection(event, tcpSource, loop);
    }

    Buffer<IO_BUFFER_SIZE> &readBuffer() {
        return readBuffer_;
    }

    Buffer<IO_BUFFER_SIZE> &writeBuffer() {
        return writeBuffer_;
    }

    void* data() {
        return data_;
    }

    void setData(void* ptr) {
        data_ = ptr;
    }

    // 向writeBuffer_中填充数据并且调用send()来确保发送最终完成
    void send(const char *str, size_t len) {
        if (state_ != State::ESTABLISHED)
            Logger::fatal("connection closed, can't send data!\n");
        writeBuffer_.append(str, len);
        send();
    }


    // 确保非阻塞地及时发送writeBuffer_中所有值
    void send(bool isLast = false);

    Event* event() {
        return event_;
    }

    Socket &socket() {
        return socket_;
    }

    // 单位是ms
    std::shared_ptr<TimerHandler> addOneTask(uint32_t time, const std::function<void()>& task) {
        return timer_.addOneTask(time, task);
    }

    // 单位是ms
    std::shared_ptr<TimerHandler> addTimedTask(uint32_t firstTime, uint32_t intervalTime, const std::function<void()>& task) {
        return timer_.addTimedTask(firstTime, intervalTime, task);
    }

    EventLoop *eventLoop() {
        return loop_;
    }

    void destroy() {
        // todo 补充点什么
        state_ = CLOSED;
        loop_->epoller()->removeEvent(event_);
        ObjectPool::returnObject<Event>(event_);
    }

    void setDestoryCallback(std::function<void(TcpConnection*)> destoryCallback) {
        destoryCallback_ = std::move(destoryCallback);
    }

    void activeClose();

    TcpConnection(TcpConnection &&other) = default;

    // fixed 无法声明默认的析构函数，否则会引起TcpServer::readCallback()函数中的connections.insert插入错误！
    // 因为之前未声明移动构造函数，而显式声明一个（即使是默认的）析构函数会将移动构造函数定义为隐式删除的！
    ~TcpConnection() = default;

private:
    void sendNonblock();

private:
    Buffer<IO_BUFFER_SIZE> readBuffer_, writeBuffer_; // read和write是相对于用户而言
    void *data_ = nullptr;
    Event* event_; // 当前连接对应的epoller中的event，todo 记得delete
    Socket socket_;
    bool isWillClose_; // todo 使用state_代替之
    std::function<void(TcpConnection*)>destoryCallback_; // only for FreeServerClient
    State state_;
    // 保存Tcpserver的引用是为了调用tcpServer_->closeConnection
    TcpSource *tcpSource_;
    // 保存EventLoop的引用是为了调用loop_->epoller()或指明谁在控制此连接
    EventLoop *loop_;
    Timer timer_;
};


#endif //TESTLINUX_TCPCONNECTION_H
