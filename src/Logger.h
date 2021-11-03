//
// Created by Hello Peter on 2021/7/18.
//

#ifndef MUDUO_MY_LOGGER_H
#define MUDUO_MY_LOGGER_H

#include <thread>
#include <mutex>
#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <vector>
#include <iostream>
#include <condition_variable>
#include <chrono>
#include <memory>
#include <fmt/core.h>
#include <atomic>

#define EMPTYS_INIT_SIZE 4
#define EMPTYS_MAX_SIZE 20
#define BUFFER_SIZE 4096
#define MIN_SYNC_TIME 1

// singleton util
class Logger {
public:
    enum LogLevel {
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL,
        NUM_LOG_LEVELS
    };

    // TODO: thread-safe singleton
    static Logger &get_logger() {
        static Logger logger;
        return logger;
    }

    template<typename... T>
    static void info(fmt::format_string<T...> fmt, T &&... args) {
        std::string formattedString = vformat(fmt, fmt::make_format_args(args...));
        get_logger().log(INFO, formattedString.data(), formattedString.size());
    }

    template<typename... T>
    static void fatal(fmt::format_string<T...> fmt, T &&... args) {
        std::string formattedString = vformat(fmt, fmt::make_format_args(args...));
        get_logger().log(FATAL, formattedString.data(), formattedString.size());
    }

    template<typename... T>
    static void sys(fmt::format_string<T...> fmt, T &&... args) {
        std::string formattedString = vformat(fmt, fmt::make_format_args(args...));
        formattedString += vformat(" because {}\n", fmt::make_format_args(strerror(errno)));
        get_logger().log(ERROR, formattedString.data(), formattedString.size());
    }

    void sync() const {
        if (outputFd_ != -1)
            fsync(outputFd_);
        if (this->buffers_.empty() && (!this->currentBuffer_ || this->currentBuffer_->empty()))
            std::clog << "sync确实结束了" << std::endl;
    }

    LogLevel logLevel() const {
        return level_;
    }

    void setLogLevel(LogLevel level) {
        level_ = level;
    }

    void setOutputFile(const char *filename, bool trunc);

//    template<typename... T>
//    void log(LogLevel logLevel, fmt::format_string<T...> fmt, T &&... args) {
//        std::string formattedString = vformat(fmt, fmt::make_format_args(args...));
//        log(logLevel, formattedString.data(), formattedString.size());
//    }

    void log(LogLevel level, const char *logline);

    void log(LogLevel level, const char *logline, size_t len);

    void start();

    void stop();

    ~Logger() {
        stop();
        if (outputFd_ != STDOUT_FILENO) {
            close(outputFd_);
        }
    }

private:
    void backThread();

    Logger() : level_(INFO), outputFd_(STDOUT_FILENO), mutex_(), running(),
               currentBuffer_(new std::string), buffers_(),
               emptys_(EMPTYS_INIT_SIZE) {
        currentBuffer_->reserve(BUFFER_SIZE);
        for (auto &bufferPtr : emptys_) {
            bufferPtr.reset(new std::string);
            bufferPtr->reserve(BUFFER_SIZE);
        }
        start();
    }

    enum LogLevel level_;
    int outputFd_;

    std::mutex mutex_;
    std::condition_variable cond_, willCloseCond_;
    std::atomic_flag running;
    std::atomic<bool> isInCollectLog_ = false;

    std::unique_ptr<std::string> currentBuffer_;
    std::vector<std::unique_ptr<std::string>> buffers_;
    std::vector<std::unique_ptr<std::string>> emptys_;

};

// 函数声明可以有多份，但是定义只能存在于一个源文件（而非头文件）中
// Logger &get_logger();

#endif //MUDUO_MY_LOGGER_H
