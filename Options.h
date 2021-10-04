//
// Created by Hello Peter on 2021/9/29.
//

#ifndef TESTLINUX_OPTIONS_H
#define TESTLINUX_OPTIONS_H

#include <sys/resource.h>
#include "Logger.h"
#include <cassert>
class Options {
public:
    static size_t getMaxFiles() {
        rlimit ret;
        getrlimit(RLIMIT_NOFILE, &ret);
        Logger::info("cur: {}, max: {}", ret.rlim_cur, ret.rlim_max);
        return ret.rlim_cur;
    }

    static int setMaxFiles(size_t files) {
        rlimit ret;
        getrlimit(RLIMIT_NOFILE, &ret);
        ret.rlim_cur = ret.rlim_max;
        return setrlimit(RLIMIT_NOFILE, &ret);
    }
};

#endif //TESTLINUX_OPTIONS_H
