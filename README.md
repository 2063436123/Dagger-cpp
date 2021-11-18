## Dagger-cpp

### Overview
基于cpp11的轻量级网络库。

目前支持Linux平台和TCP/IPv4协议。

### 架构图
![](../../../Desktop/Dagger架构图.png)

### Benchmark
[here](docs/benchmark.md)

### Feature highlights
- epoll实现高性能IO多路复用
- 无阻塞的消息收发
- EventLoopPool
- 异步的多线程日志库
- 空闲连接管理

### Build
```cpp
mkdir build && cmake ..
make
```

### 后续计划
- 增加UDP支持
- 增加DNS支持
- 增加openssl支持
- 实现chat、broadcast、rpc等使用场景样例
- 优化性能