//
// Created by Hello Peter on 2021/11/3.
//
#include "../src/SpinLock.h"
#include <thread>
#include <gtest/gtest.h>
#include <vector>

int i = 0;
SpinLock g_lock;

void threadFunc() {
    g_lock.lock();
    for (int k = 0; k < 1000; k++) {
        i++;
        usleep(10);
    }
    g_lock.unlock();
}

std::vector<int> vec;
void threadFunc2() {
    g_lock.lock();
    for (int i = 0; i < 100; i++) {
        vec.push_back(rand());
        usleep(10);
    }
    g_lock.unlock();
}

TEST(SpinLock, Basic) {
    std::thread t1(threadFunc), t2(threadFunc);
    t1.join();
    t2.join();
    ASSERT_EQ(i, 10000 * 2);
}

TEST(SpinLock, Container) {
    std::thread t1(threadFunc2), t2(threadFunc2);
    t1.join();
    t2.join();
    ASSERT_EQ(vec.size(), 200);
}

int main() {
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}

