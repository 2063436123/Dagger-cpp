//
// Created by Hello Peter on 2021/11/3.
//
#include "../src/InAddr.h"
#include <string.h>
#include <gtest/gtest.h>

TEST(InAddrTest, Basic) {
    InAddr addr1("6667", "127.0.0.1", 2);
    std::string port("6667"), addr("127.0.0.1");
    InAddr addr2(port, addr);
    EXPECT_EQ(addr1.ipPortStr(), addr2.ipPortStr());
    EXPECT_FALSE(memcmp(addr1.sockAddr(), addr2.sockAddr(), sizeof(sockaddr)));

    in_addr addr0;
    inet_pton(2, "127.0.0.1", &addr0);
    in_port_t port0 = htons(6667);
    InAddr addr3(port0, addr0);
    EXPECT_EQ(addr1.ipPortStr(), addr3.ipPortStr());
    EXPECT_FALSE(memcmp(addr1.sockAddr(), addr3.sockAddr(), sizeof(sockaddr)));
}

int main() {
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}

