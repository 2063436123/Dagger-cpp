//
// Created by Hello Peter on 2021/7/21.
//
#include <cstring>
#include <string>
#include <sys/time.h>
#include "../Logger.h"

uint32_t fullBufferCnt;
uint32_t enterLogCnt;

void bench() {
  auto start = std::chrono::steady_clock::now();

  size_t kBench = 10 * 10000; // message条数
  std::string message = "Hello logger: msg number ";

  for (size_t i = 1; i <= kBench; i++) {
    std::string msg = "[async_logger] [info] " + message + std::to_string(i) + '\n';
    Logger::get_logger().log(Logger::INFO, msg.data(), msg.size());
  }
  Logger::get_logger().sync();

  auto end = std::chrono::steady_clock::now();
  std::cout << "total_time: " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << "ns"
            << std::endl;

  std::cout << "speed: " << static_cast<double>(kBench) /
      (static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()) / (1000 * 1000))
      << " msg/s" << std::endl;
}

int main() {
  Logger &logger = Logger::get_logger();
  logger.setOutputFile("/tmp/my_logger.log", true);
  logger.start();
  for (int i = 0; i < 10; i++)
    bench();
  logger.stop();
}
