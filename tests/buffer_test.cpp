//
// Created by Hello Peter on 2021/10/30.
//

#include "../src/Buffer.h"
#include <iostream>
using namespace std;


int main() {
    Buffer<8192> buffer;
    int n = 10;
    std::string str(16 * 1024, ' ');
    while (n--) {
        buffer.append(str.c_str(), str.size());
        cout << buffer.readableBytes() << " " << buffer.writableBytes() << endl;
        buffer.retrieveAll();
    }
}
