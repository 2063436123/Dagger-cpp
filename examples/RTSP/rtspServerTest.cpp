//
// Created by Hello Peter on 2021/11/3.
//
#include "./RtspServer.h"

int main() {
    RtspServer server(InAddr("0.0.0.0:554"));
    server.start(3);
}
