//
// Created by Hello Peter on 2021/10/31.
//

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>

using namespace std;
using boost::asio::ip::tcp;
using boost::asio::buffer;


class AsioIOServicePool {
public:
    using IOService = boost::asio::io_service;
    using Work = boost::asio::io_service::work;
    using WorkPtr = std::unique_ptr<Work>;

    AsioIOServicePool(std::size_t size = std::thread::hardware_concurrency())
            : ioServices_(size),
              works_(size),
              nextIOService_(0) {
        for (std::size_t i = 0; i < size; ++i) {
            works_[i] = std::unique_ptr<Work>(new Work(ioServices_[i]));
        }
        for (std::size_t i = 0; i < ioServices_.size(); ++i) {
            threads_.emplace_back([this, i]() {
                ioServices_[i].run();
            });
        }
    }

    AsioIOServicePool(const AsioIOServicePool &) = delete;

    AsioIOServicePool &operator=(const AsioIOServicePool &) = delete;

    // 使用 round-robin 的方式返回一个 io_service
    boost::asio::io_service &getIOService() {
        auto &service = ioServices_[nextIOService_++];
        if (nextIOService_ == ioServices_.size()) {
            nextIOService_ = 0;
        }
        return service;
    }

    void stop() {
        for (auto &work: works_) {
            work.reset();
        }
        for (auto &t: threads_) {
            t.join();
        }
    }

private:
    std::vector<IOService> ioServices_;
    std::vector<WorkPtr> works_;
    std::vector<std::thread> threads_;
    std::size_t nextIOService_;
};


class Connection : public std::enable_shared_from_this<Connection> {
public:
    Connection(tcp::socket s) : socket_(std::move(s)) {}

    void start() {
        async_read();
    }

    tcp::socket &socket() {
        return socket_;
    }

private:
    void async_read() {
        auto self(shared_from_this());
        socket_.async_read_some(buffer(data_),
                                [this, self](const boost::system::error_code &ec, size_t bytes_transferred) {
                                    if (!ec)
                                        async_write(bytes_transferred);
                                });
    }

    void async_write(std::size_t length) {
        auto self(shared_from_this());
        boost::asio::async_write(socket_, buffer(data_, length),
                                 [this, self](const boost::system::error_code &ec, size_t) {
                        // 1. for ping-pong
//                                     if (!ec)
//                                         async_read();
                        // 2. for webbench
                                     socket_.close();
                                 });
    }


    tcp::socket socket_;
    std::array<char, 2048> data_;
};

AsioIOServicePool pool(3);

class Server {
public:
    Server(boost::asio::io_service &io_service, short port) : acceptor_(io_service, tcp::endpoint(tcp::v4(), port)) {
        async_accept();
    }

private:
    void async_accept() {
        std::shared_ptr<Connection> session_ptr(new Connection(tcp::socket(pool.getIOService())));
        acceptor_.async_accept(session_ptr->socket(), [this, session_ptr](const boost::system::error_code &ec) {
            if (!ec) {
                session_ptr->start(); // 在一个随机选出的固定的service线程中执行
            }
            async_accept();
        });
    }

    tcp::acceptor acceptor_;
};

int main() {

    boost::asio::io_service io_service;
    Server server(io_service, 12345);
    io_service.run();
}