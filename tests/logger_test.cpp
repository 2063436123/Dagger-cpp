//
// Created by Hello Peter on 2021/7/21.
//
#include <cstring>
#include <string>
#include <gtest/gtest.h>
#include <sys/time.h>
#include "../src/Logger.h"

uint32_t fullBufferCnt;
uint32_t enterLogCnt;

TEST(LoggerTest, Basic) {
    Logger &logger = Logger::get_logger();
    logger.setOutputFile("/tmp/my_logger.log", true);
    logger.start();

    logger.log(Logger::DEBUG, "tmp", 3);
    logger.log(Logger::WARN, "atank\n");

    logger.stop();

    int fd = open("/tmp/my_logger.log", O_RDONLY);
    char buf[256];
    int n = read(fd, buf, 255);
    buf[n] = '\0';
    puts(buf);
    char* pre = strchr(buf, ']');
    ASSERT_NE(pre, nullptr);
    ASSERT_NE(strstr(pre, "atank"), nullptr);
}


int main() {
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
