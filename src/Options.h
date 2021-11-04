//
// Created by Hello Peter on 2021/9/29.
//

#ifndef TESTLINUX_OPTIONS_H
#define TESTLINUX_OPTIONS_H


#include <unistd.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <linux/sysctl.h>
#include "Logger.h"
#include <cassert>

#if __GNUC__
#if __x86_64__ || __ppc64__
#define ENVIRONMENT64
#else
#define ENVIRONMENT32
#endif
#endif

// 定义DataType是为了兼容32/64位平台，这样就可以安全地在void*和integer_type之间转换
#ifdef ENVIRONMENT64
using DataType = uint64_t;
#elif
using DataType = uint32_t;
#endif

class Options {
public:
    static size_t getMaxFiles() {
        rlimit ret;
        getrlimit(RLIMIT_NOFILE, &ret);
        Logger::info("cur: {}, max: {}\n", ret.rlim_cur, ret.rlim_max);
        return ret.rlim_cur;
    }

    static int setMaxFiles(size_t files) {
        rlimit ret;
        getrlimit(RLIMIT_NOFILE, &ret);
        ret.rlim_cur = ret.rlim_max;
        return setrlimit(RLIMIT_NOFILE, &ret);
    }

    static void setTcpRmem(size_t rmem) {
        std::string prefix("sudo sysctl -w net.ipv4.tcp_rmem=");
        prefix += std::to_string(rmem);
        // assert: system中执行sysctl -w能影响当前进程吗？
        int n = system(prefix.c_str());
        if (n < 0)
            Logger::sys("system error");
    }

    static void setTcpWmem(size_t wmem) {
        std::string prefix("sudo sysctl -w net.ipv4.tcp_wmem=");
        prefix += std::to_string(wmem);
        // assert: system中执行sysctl -w能影响当前进程吗？
        int n = system(prefix.c_str());
        if (n < 0)
            Logger::sys("system error");
    }
};

#endif //TESTLINUX_OPTIONS_H
