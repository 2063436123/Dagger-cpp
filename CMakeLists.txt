cmake_minimum_required(VERSION 3.16)
project(testLinux)

set(CMAKE_CXX_STANDARD 20)

add_executable(Example1 tests/example1.cpp Sources/Socket.cpp
        Sources/Epoller.cpp Sources/TcpConnection.cpp
        EventLoop.cpp EventLoop.h
        EventLoopPool.cpp EventLoopPool.h)
target_link_libraries(Example1 pthread)