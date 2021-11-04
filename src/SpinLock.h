//
// Created by Hello Peter on 2021/10/21.
//

#ifndef TESTLINUX_SPINLOCK_H
#define TESTLINUX_SPINLOCK_H

#include <atomic>
#include <cassert>
#include <thread>

class SpinLock {
    const int kMaxActiveSpin = 5000;
public:
    enum {
        FREE = 0, LOCKED = 1
    };

    SpinLock() : lock_(FREE) {}

    void lock() {
        uint32_t spinCount = 0;
        while (!cas(FREE, LOCKED)) {
            do {
                spin_wait(spinCount);
            } while (lock_.load() == LOCKED);
        }
        assert(lock_.load() == LOCKED);
    }

    std::atomic<int> state = 0;

    void unlock() {
        assert(lock_.load() == LOCKED);
        lock_.store(FREE);
    }

private:
    void spin_wait(uint32_t &spinCount) {
        if (spinCount < kMaxActiveSpin) {
            ++spinCount;
            asm volatile("pause");
        } else {
            std::this_thread::sleep_for(std::chrono::microseconds (800));
        }
    }

    bool cas(uint8_t compare, uint8_t newVal) noexcept {
        return std::atomic_compare_exchange_strong(&lock_, &compare, newVal);
    }

    std::atomic<uint8_t> lock_;
};

#endif //TESTLINUX_SPINLOCK_H
