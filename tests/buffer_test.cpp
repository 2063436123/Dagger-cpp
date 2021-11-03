//
// Created by Hello Peter on 2021/10/30.
//

#include "../src/Buffer.h"
#include <gtest/gtest.h>
#include <iostream>
using namespace std;

TEST(BufferTest, Basic) {
    Buffer<1024> buffer;
    EXPECT_EQ(buffer.readableBytes(), 0);
    EXPECT_EQ(buffer.writableBytes(), 1024);
    std::string str(1024, ' ');
    buffer.append(str.c_str(), str.size());
    EXPECT_EQ(buffer.readableBytes(), 1024);
    EXPECT_EQ(buffer.writableBytes(), 0);
    EXPECT_EQ(buffer.findStr(" "), buffer.peek());
    EXPECT_EQ(str, std::string(buffer.peek(), buffer.readableBytes()));
    buffer.retrieve(100);
    EXPECT_EQ(buffer.readableBytes(), 1024 - 100);
    EXPECT_EQ(buffer.writableBytes(), 0);
    str.resize(100);
    buffer.append(str.c_str(), str.size());
    EXPECT_EQ(buffer.readableBytes(), 1024);
    EXPECT_EQ(buffer.writableBytes(), 0);
    EXPECT_EQ(buffer.totalSize(), 1024);
    buffer.append(" ", 1);
    EXPECT_EQ(buffer.totalSize(), 2048);
    str.resize(1023);
    buffer.append(str.c_str(), str.size());
    EXPECT_EQ(buffer.totalSize(), 2048);
    EXPECT_EQ(buffer.readableBytes(), 2048);
    EXPECT_EQ(buffer.writableBytes(), 0);
}

TEST(BufferTest, LargeData) {
    Buffer<8192> buffer;
    std::string str(16 * 1024, 'a');
    int n = 10;
    while (n--) {
        buffer.append(str.c_str(), str.size());
        EXPECT_EQ(std::string(str.size(), 'a'), std::string(buffer.peek(), buffer.readableBytes()));
        buffer.retrieveAll();
    }
    EXPECT_GE(buffer.totalSize(), str.size());
}


int main() {
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
