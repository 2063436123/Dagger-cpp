//
// Created by Hello Peter on 2021/7/18.
//

#include <sys/time.h>
#include <cstring>
#include <cassert>
#include "../Logger.h"

using namespace std;

extern uint32_t fullBufferCnt;
extern uint32_t enterLogCnt;

void Logger::setOutputFile(const char *filename, bool trunc) {
  // TODO: 等待已有数据传输到磁盘完成
  if (outputFd_ != STDOUT_FILENO) {
    fsync(outputFd_);
    close(outputFd_);
  }
  int flags = O_CREAT | O_APPEND | O_WRONLY;
  if (trunc)
    flags |= O_TRUNC;

  int fd = open(filename, flags, 0600);
  if (fd < 0) {
    cerr << "open " << filename << "error!" << endl;
    exit(1);
  }
  outputFd_ = fd;
}

void prepend_msg_for_test(string& msg) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  struct tm* specNow = gmtime(&tv.tv_sec);
  char buf[128];
  if (strftime(buf, 128, "[%Y-%m-%d %H:%M:%S.", specNow) == 0) {
    cerr << "strftime error!" << endl;
    exit(1);
  }
  string header = buf + to_string(tv.tv_usec) + "] ";
  msg = header + msg;
}

void Logger::log(Logger::LogLevel level, const char *logline) {
    log(level, logline, strlen(logline));
}

void Logger::log(LogLevel level, const char *logline, size_t len) {
  if (level < level_ || level >= NUM_LOG_LEVELS) {
    return;
  }

  std::string msg(logline, len);
  prepend_msg_for_test(msg);

  lock_guard<mutex> lg(mutex_);
  // 注意if判断条件，currentBuffer_可能为空
  if (!currentBuffer_ || BUFFER_SIZE - currentBuffer_->size() < msg.size()) {
    // 当前缓冲区不足，需要更换新的缓冲区

    if (currentBuffer_)
      buffers_.push_back(std::move(currentBuffer_));
    if (emptys_.empty()) {
      currentBuffer_.reset(new std::string);
      currentBuffer_->reserve(BUFFER_SIZE);
    } else {
      currentBuffer_ = std::move(emptys_.back());
      emptys_.pop_back();
    }
  }
  *currentBuffer_ += msg;

  if (!isInCollectLog_)
    cond_.notify_one();
}

void Logger::backThread() {
  while (running.test_and_set()) {
    // 1. 准备资源
    decltype(buffers_) fullBuffers;
    decltype(emptys_) newEmptys(EMPTYS_INIT_SIZE);
    for (auto &ptr : newEmptys) {
      ptr.reset(new std::string);
      ptr->reserve(BUFFER_SIZE);
    }

    // 2. 收回buffers_，补充emptys_
    { // Critical Section
      unique_lock<mutex> ul(mutex_);
      if (buffers_.empty()) {
        // 只等待一定时间就退出（而不是while + cond_.wait()），
        // 是为了及时回收currentBuffer_缓冲区
        cond_.wait_for(ul, chrono::milliseconds(1000 * MIN_SYNC_TIME));
      }
      isInCollectLog_ = true;
      if (currentBuffer_)
        buffers_.push_back(std::move(currentBuffer_));
      fullBuffers.swap(buffers_);
      isInCollectLog_ = false;

      uint32_t i = 0;
      while (emptys_.size() < EMPTYS_MAX_SIZE && i < EMPTYS_INIT_SIZE) {
        emptys_.push_back(std::move(newEmptys[i++]));
      }
    }

    // 3. 将buffers_写回
    for (auto &bufferPtr : fullBuffers)
      write(outputFd_, bufferPtr->c_str(), bufferPtr->size());
    fsync(outputFd_);
  }
  willCloseCond_.notify_one();
}

void Logger::start() {
  running.test_and_set();
  thread bgWorkThread(&Logger::backThread, this);
  bgWorkThread.detach();
}

void Logger::stop() {
  running.clear();
  unique_lock<mutex> ul(mutex_);
  willCloseCond_.wait(ul);
}

