//
// Created by Hello Peter on 2021/11/3.
//

#ifndef TESTLINUX_HTTPSERVER_H
#define TESTLINUX_HTTPSERVER_H

#include <utility>
#include <functional>
#include "TcpServer.h"

// for http request parsing
struct HttpRequest {
    const char *method{};
    const char *url{};
    const char *version{};
    std::vector<std::pair<const char *, const char *>> headers; // key和value都以'\0'结尾
    // 暂时不处理带有body的http请求
    //uint32_t body_length{};
    //char *body{};
};

struct HttpResponse {
    std::string response_line = "HTTP/1.1 200 OK\r\n";
    std::string headers = "content-type: text/html; charset=utf-8\r\n"
                          "connection: close\r\n";
    std::string response_body;
    void addHeader(const char* key, const char* value) {
        headers += key;
        headers += ": ";
        headers += value;
        headers += "\r\n";
    }

    void addBody(const char* str, size_t len) {
        std::string integer = std::to_string(len);
        addHeader("content-length", integer.c_str());
        response_body = std::string(str, len);
    }

    void addBody(const std::string& body) {
        addBody(body.c_str(), body.size());
    }

    [[nodiscard]] std::string to_string() const {
        return response_line + headers + "\r\n" + response_body;
    }

};

class HttpServer {
public:
    using serviceHandler = std::function<void(TcpConnection *)>;

    explicit HttpServer(InAddr bindAddr) : codec_(Codec::HTTP_MODEL, 0),
                                           server_(Socket::makeNewSocket(), bindAddr, &loop_, codec_) {
        server_.setConnMsgCallback([this](TcpConnection *conn) { this->handleHttpRequest(conn); });
    }

    void start(int workerNums = 0) {
        server_.start(workerNums);
    }

    // 添加用户自定义服务，会覆盖同名服务
    void addServiceHandler(const std::string &service_name, serviceHandler handler) {
        services_[service_name] = std::move(handler);
    }

    ~HttpServer() {
        server_.stop();
    }

private:
    void readField(char *&ptr, char ch = ' ') {
        while (*ptr != ch)
            ++ptr;
        *ptr++ = '\0';
        while (*ptr == ch)
            ++ptr;
    }

    void readLastField(char *&ptr) {
        while (*ptr != '\r' && *ptr != ' ')
            ++ptr;
        *ptr++ = '\0';
        while (*ptr == ' ')
            ++ptr;
        if (*ptr == '\r')
            ++ptr;
        ++ptr;
    }

    void handleHttpRequest(TcpConnection *conn) {
        HttpRequest httpMetaData;
        // parse, rule in here: https://en.wikipedia.org/wiki/Hypertext_Transfer_Protocol#Request_syntax
        char *ptr = conn->readBuffer().peek();

        httpMetaData.method = ptr;
        readField(ptr);
        httpMetaData.url = ptr;
        readField(ptr);
        httpMetaData.version = ptr;
        readLastField(ptr);
        while (*ptr != '\r') {
            std::pair<const char *, const char *> kv;
            kv.first = ptr;
            readField(ptr, ':');
            while (*ptr == ' ')
                ++ptr;
            kv.second = ptr;
            readLastField(ptr);
            httpMetaData.headers.push_back(kv);
        }
        while (*ptr != '\0')
            ++ptr;
        ++ptr;

        size_t readLen = ptr - conn->readBuffer().peek();
        conn->readBuffer().retrieve(readLen); // note: 被retrieve的数据还在被httpMetaData引用，所以send必须在最后调用
        //Logger::info("method:{}, url:{}, version:{}, headers_size:{}, readLen:{}\n",
        //             httpMetaData.method, httpMetaData.url, httpMetaData.version, httpMetaData.headers.size(), readLen);
        //Logger::info("{}:{}\n{}:{}\n", httpMetaData.headers[0].first, httpMetaData.headers[0].second,
        //             httpMetaData.headers[1].first, httpMetaData.headers[1].second);

        std::string response("blank");
        // do work
        auto ret = services_.find(httpMetaData.url);
        if (ret != services_.end()) {
            ret->second.operator()(conn);
            return;
        } else {
            if (strcmp(httpMetaData.url, "/") == 0) {
                HttpResponse response0;
                std::string body("<!DOCTYPE html>\n"
                                 "<html>\n"
                                 "<head>\n"
                                 "<title>Welcome to nginx!</title>\n"
                                 "<style>\n"
                                 "body {\n"
                                 "width: 35em;\n"
                                 "margin: 0 auto;\n"
                                 "font-family: Tahoma, Verdana, Arial, sans-serif;\n"
                                 "}\n"
                                 "</style>\n"
                                 "</head>\n"
                                 "<body>\n"
                                 "<h1>Welcome to nginx!</h1>\n"
                                 "<p>If you see this page, the nginx web server is successfully installed and\n"
                                 "working. Further configuration is required.</p>\n"
                                 "\n"
                                 "<p>For online documentation and support please refer to\n"
                                 "<a href=\"http://nginx.org/\">nginx.org</a>.<br/>\n"
                                 "Commercial support is available at\n"
                                 "<a href=\"http://nginx.com/\">nginx.com</a>.</p>\n"
                                 "\n"
                                 "<p><em>Thank you for using nginx.</em></p>\n"
                                 "</body>\n"
                                 "</html>");
                response0.addBody(body);
                response = response0.to_string();
            } else {
                response = "HTTP/1.1 400 Bad Request\r\n"
                           "content-type: text/html; charset=utf-8\r\n"
                           "content-length: 49\r\n"
                           "connection: close\r\n"
                           "\r\n"
                           "<title>The Service haven't been provided.</title>\r\n";
            }
        }

        // response
        conn->send(response.c_str(), response.size());
        conn->activeClose();
    }

    // 存储用户自定义服务类型，例如"/threadnums":`a function response the number of threads`
    std::unordered_map<std::string, serviceHandler> services_;
    EventLoop loop_;
    Codec codec_;
    TcpServer server_;

};

#endif //TESTLINUX_HTTPSERVER_H

#pragma clang diagnostic pop