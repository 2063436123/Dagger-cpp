//
// Created by Hello Peter on 2021/11/4.
//
#include "../ObjectPool.hpp"
#include "../TcpConnection.h"
std::vector<char *> blocks;
void release() {
    for (char* block : blocks)
        delete[] block;
}