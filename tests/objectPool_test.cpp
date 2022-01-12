//
// Created by Hello Peter on 2021/11/4.
//

#include "../src/ObjectPool.hpp"
#include <gtest/gtest.h>
#include <string>

using namespace std;

class Base {
public:
    Base(int a) : a_(a) {}
    int a_;
};

TEST(ObjectPoolTest, Basic) {
    ObjectPool::addObjectCache<std::string>(2);
    string* str1 = ObjectPool::getNewObject<std::string>("hello");
    string* str2 = ObjectPool::getNewObject<std::string>("world");
    ObjectPool::addObjectCache<std::string>(1);
    string* str3 = ObjectPool::getNewObject<std::string>("unit_amount");
    EXPECT_NE(str1, str2);
    EXPECT_NE(str1, str3);
    EXPECT_EQ(*str1, std::string("hello"));
    EXPECT_EQ(*str2, std::string("world"));
    EXPECT_EQ(*str3, std::string("unit_amount"));
}

TEST(ObjectPoolTest, Class) {
    Base* b = ObjectPool::getNewObject<Base>(10);
    EXPECT_EQ(b->a_, 10);
}

int main() {
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}