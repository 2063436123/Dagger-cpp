//
// Created by Hello Peter on 2021/10/31.
//
#include <iostream>
#include <memory>
#include <array>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;
using boost::asio::buffer;


class Connection : public std::enable_shared_from_this<Connection> {
public:
    Connection(tcp::socket s) : socket_(std::move(s)) {}

    void start() {
        async_read();
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
                                     if (!ec)
                                         async_read();
                                     // 2. for webbench
//                                     socket_.close();
                                 });
    }


    tcp::socket socket_;
    std::array<char, 2048> data_;
};

class server {
public:
    server(boost::asio::io_service &io_service, short port) : acceptor_(io_service, tcp::endpoint(tcp::v4(), port)),
                                                              socket_(io_service) {
        async_accept();
    }

private:
    void async_accept() {
        acceptor_.async_accept(socket_, [this](const boost::system::error_code &ec) {
            handle_accept(ec);
        });
    }

    void handle_accept(const boost::system::error_code &ec) {
        if (!ec) {
            std::shared_ptr<Connection> session_ptr(new Connection(std::move(socket_)));
            session_ptr->start();
        }
        async_accept();
    }


    tcp::acceptor acceptor_;
    tcp::socket socket_;
};

int main() {
    boost::asio::io_service io_service;
    server s(io_service, 12345);
    io_service.run();
}