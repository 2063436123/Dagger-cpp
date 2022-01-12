//
// Created by Hello Peter on 2021/12/22.
//

#include "RtspServer.h"
#include <sys/time.h>

using namespace std;

static string parseFieldAndBlanks(const char *&p) {
    const char *old_p = p;
    while (*p != 0 && *p != ' ')
        ++p;
    string ret(old_p, p - old_p);
    while (*p == ' ')
        ++p;
    return ret;
}

static string parseFieldAndColon(const char *&p) {
    const char *old_p = p;
    while (*p != 0 && *p != ':' && *p != '\r')
        ++p;
    if (*p == '\r')
        return "";
    string ret(old_p, p - old_p);
    while (*p == ':' || *p == ' ')
        ++p;
    return ret;
}

static string parseFieldAndCRLF(const char *&p) {
    const char *old_p = p;
    while (*p != 0 && *p != '\r')
        ++p;
    string ret(old_p, p - old_p);
    while (*p == ' ')
        ++p;
    if (*p == '\r')
        ++p;
    if (*p == '\n')
        ++p;
    return ret;
}

RtspRequest parseRequest(const char *&p) {
    assert(p);
    RtspRequest request;
    request.method = parseFieldAndBlanks(p);
    request.URI = parseFieldAndBlanks(p);
    request.version = parseFieldAndCRLF(p);
    string key, value;
    do {
        key = parseFieldAndColon(p);
        if (key.empty())
            break;
        value = parseFieldAndCRLF(p);
        request.headers.insert({key, value});
    } while (true);
    // todo 判断是否有消息体 + body设置
    return request;
}

void debugRequest(const RtspRequest &request) {
    cout << "解析后的request:\n[";
    cout << request.method << '\n';
    cout << request.URI << '\n';
    cout << request.version << '\n';
    for (auto pair: request.headers)
        cout << pair.first << ":" << pair.second << '\n';
    cout << '\n' << request.body << ']' << endl;
}

string strifyResponse(const RtspResponse &response) {
    char buf[512];
    int len = sprintf(buf, "%s %s %s\r\n", response.version.c_str(), response.status_code.c_str(),
                      response.status_cause.c_str());
    string ret(buf, len);
    for (const auto &kv: response.headers)
        ret += kv.first + ": " + kv.second + "\r\n";
    ret += "\r\n";
    if (!response.body.empty())
        ret += response.body + "\r\n";
    return ret;
}

mutex session_mutex;
unordered_map<std::string, shared_ptr<SessionState>> sessions;

string createSessionId() {
    string str(10, ' ');
    for (int i = 0; i < 10; i++)
        str[i] = rand() % 10 + '0';
    return str;
}

string nowDate() {
    timeval tv;
    gettimeofday(&tv, nullptr);
    struct tm *specNow = gmtime(&tv.tv_sec);
    char buf[128];
    if (strftime(buf, 128, "%d %b %Y %H:%M:%S GMT", specNow) == 0) {
        cerr << "strftime error!" << endl;
        exit(1);
    }
    return {buf};
}

RtspResponse handleRequest(const RtspRequest &request) {
    RtspResponse response;
    // 通用参数值，可覆盖
    response.version = request.version;
    response.status_code = "200";
    response.status_cause = "OK";
    auto cseq = request.headers.find("CSeq");
    assert(cseq != request.headers.end());
    response.headers.insert({"CSeq", cseq->second});

    if (request.method == "OPTIONS") {
        response.headers.insert({"Public", "DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE"});
    } else if (request.method == "SETUP") {
        response.headers.insert({"Date", nowDate()});
        string sessionId = createSessionId();
        {
            lock_guard<mutex> lg(session_mutex);
            auto ret = sessions.insert({sessionId, make_shared<SessionState>(sessionId, request.URI, request.version)});
            assert(ret.second == true);
        }
        response.headers.insert({"Session", sessionId});
        auto transport = request.headers.find("Transport");
        assert(transport != request.headers.end());
        response.headers.insert({"Transport", transport->second + ";server_port=554-555"});
    } else if (request.method == "TEARDOWN") {
        // todo request中没有session怎么办
        auto iter = request.headers.find("Session");
        assert(iter != request.headers.end());
        {
            lock_guard<mutex> lg(session_mutex);
            auto iter2 = sessions.find(iter->second);
            assert(iter2 != sessions.end());
            iter2->second->cur_state = DESTORYED;
            sessions.erase(iter2);
        }
    } else {
        response.status_code = "455";
        response.status_cause = "Method Not Valid in This State";
    }
    return response;
}

void RtspServer::handleRtspRequest(TcpConnection *conn) {
    std::cout << "received: \n[" << std::string(conn->readBuffer().peek(), conn->readBuffer().readableBytes()) << "]"
              << std::endl;
    const char *p = conn->readBuffer().peek();
    const char *old_p = p;
    conn->readBuffer().append("\0", 1);
    RtspRequest request = parseRequest(p);
//    debugRequest(request);
    cout << "will retrieve " << old_p - p << endl;
    conn->readBuffer().retrieve(old_p - p);
    RtspResponse response = handleRequest(request);
    string responseStr = strifyResponse(response);
    std::cout << "sent: \n[" << responseStr << ']' << std::endl;
    conn->send(responseStr.c_str(), responseStr.size());
}
