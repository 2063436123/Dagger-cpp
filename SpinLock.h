//
// Created by Hello Peter on 2021/10/21.
//

#ifndef TESTLINUX_SPINLOCK_H
#define TESTLINUX_SPINLOCK_H

class SpinLock {
public:
    void lock() {
        mutex_.lock();
    }
    void unlock() {
        mutex_.unlock();
    }

private:
    // todo 临时替代，需要替换成真正的自旋锁（folly可借鉴）
    std::mutex mutex_;
};

#endif //TESTLINUX_SPINLOCK_H
