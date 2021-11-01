//
// Created by Hello Peter on 2021/10/31.
//

#include "../../src/Socket.h"
#include "../../src/Buffer.h"
#include "../../src/InAddr.h"
#include <openssl/ssl.h>
#include <openssl/err.h>

SSL *ssl;

void log_ssl()
{
    int err;
    while (err = ERR_get_error()) {
        char *str = ERR_error_string(err, 0);
        if (!str)
            return;
        puts(str);
        printf("\r\n");
        fflush(stdout);
    }
}

void openssl_init() {
    SSL_library_init();
    SSLeay_add_ssl_algorithms();
    SSL_load_error_strings();
    const SSL_METHOD *meth = TLSv1_2_client_method();
    SSL_CTX *ctx = SSL_CTX_new(meth);
    ssl = SSL_new(ctx);
    if (!ssl) {
        printf("Error creating SSL.\r\n");
        log_ssl();
    }
}

int main() {
    Socket s = Socket::makeNewSocket();
    InAddr addr("44336", "127.0.0.1");
    s.connect(addr);

    openssl_init();

    int sock = SSL_get_fd(ssl);
    SSL_set_fd(ssl, s.fd());
    int err = SSL_connect(ssl);
    assert(err > 0);
    printf ("SSL connection using %s\n", SSL_get_cipher (ssl));

    const char *request = "GET /s?wd=abc HTTP/1.1\r\n"
                          "Host: www.baidu.com\r\n"
                          "Connection: keep-alive\r\n"
                          "Cache-Control: max-age=0\r\n"
                          "sec-ch-ua: \"Google Chrome\";v=\"95\", \"Chromium\";v=\"95\", \";Not A Brand\";v=\"99\"\r\n"
                          "sec-ch-ua-mobile: ?0\r\n"
                          "sec-ch-ua-platform: \"macOS\"\r\n"
                          "DNT: 1\r\n"
                          "Upgrade-Insecure-Requests: 1\r\n"
                          "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/95.0.4638.54 Safari/537.36\r\n"
                          "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9\r\n"
                          "Sec-Fetch-Site: none\r\n"
                          "Sec-Fetch-Mode: navigate\r\n"
                          "Sec-Fetch-User: ?1\r\n"
                          "Sec-Fetch-Dest: document\r\n"
                          "Accept-Encoding: gzip, deflate, br\r\n"
                          "Accept-Language: zh-CN,zh;q=0.9\r\n";
    int len = SSL_write(ssl, request, strlen(request));
    assert(len == strlen(request));

    char buf[10000];
    len = SSL_read(ssl, buf, 10000);
    buf[len] = '\0';
    printf("receive: %s\n", buf);
}