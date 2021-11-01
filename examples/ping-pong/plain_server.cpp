#include "../../src/Socket.h"
#include "../../src/Buffer.h"
#include "../../src/InAddr.h"

int main() {
    Socket s = Socket::makeNewSocket();
    s.bindAddr(InAddr("12345"));
    s.listen(5);
    Socket connectedSocket = Socket::makeConnected(s.accept());

    char buf[2048];
    while (true) {
        int fd = connectedSocket.fd();
        int n = read(fd, buf, sizeof(buf));
        if (n <= 0)
            return 0;
        write(fd, buf, n);
    }
}