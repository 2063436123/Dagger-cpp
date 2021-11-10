//
// Created by Hello Peter on 2021/11/4.
//

#ifndef TESTLINUX_OBJECTPOOL_HPP
#define TESTLINUX_OBJECTPOOL_HPP

#include <vector>
#include <list>
#include <iostream>
#include <cassert>

extern uint64_t newConnRequests;
extern uint64_t reuseRequests;
extern uint64_t runtimeConnNums;
extern uint64_t recoveryConnNums;
// 保存所有的内存块，程序退出时要全部遍历释放
extern std::vector<char *> blocks;
void release();

class ReleaseGuard {
public:
    ~ReleaseGuard() {
        release();
    }
    static ReleaseGuard guard;
};

static ReleaseGuard guard;

class ObjectPool {
public:
    template<typename T>
    static void addObjectCache(size_t num);

    template<typename T, typename... Args>
    static T *getNewObject(Args &&... args);

    template<typename T>
    static void returnObject(T *);

private:
    // todo connection_pool_的成员不用循环new，完全可以先new一块大的，再自行分隔
    template<typename T>
    static std::list<T *> pool_;

    template<typename T>
    static char *original_block_;
};

// 每种T类型共享一份pool_，将其声明为ObjectPool的成员只是为了控制只有ObjectPool的方法可以访问它.
template<typename T>
std::list<T *> ObjectPool::pool_;
// 一次性分配多个T块，供pool_指向
template<typename T>
char *ObjectPool::original_block_ = nullptr;

template<typename T>
void ObjectPool::addObjectCache(size_t num) {
    return;
    original_block_<T> = new char[num * sizeof(T)];
    blocks.push_back(original_block_<T>);
    const char *ptr = original_block_<T>;
    for (size_t i = 0; i < num; i++) {
        pool_<T>.push_back((T*)ptr);
        ptr += sizeof(T);
    }
    assert(ptr == original_block_<T> + num * sizeof(T));
}

// not thread safe
template<typename T, typename... Args>
T *ObjectPool::getNewObject(Args &&... args) {
    return new T(std::forward<Args>(args)...);
    ++newConnRequests;
    if (pool_<T>.empty()) {
        ++runtimeConnNums;
        return new T(args...);
    }
    T *location = pool_<T>.front();
    pool_<T>.pop_front();
    ++reuseRequests;
    return new(location)T(std::forward<Args>(args)...);
}

template<typename T>
void ObjectPool::returnObject(T *object) {
    delete object;
    return;
    ++recoveryConnNums;
    object->~T();
    pool_<T>.push_back(object);
}

#endif //TESTLINUX_OBJECTPOOL_HPP
