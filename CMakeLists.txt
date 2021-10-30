cmake_minimum_required(VERSION 3.16)
project(testLinux)

set(CMAKE_CXX_STANDARD 20)

link_libraries(pthread)
include_directories(third_party/fmt_include)

set(SRCS src/Sources/Socket.cpp
        src/Sources/Epoller.cpp src/Sources/TcpConnection.cpp
        src/Sources/EventLoop.cpp
        src/Sources/Logger.cpp
        third_party/fmt_src/os.cc third_party/fmt_src/format.cc
        src/Sources/TcpServer.cpp)

# tcp_server test
add_executable(example_server tests/example_server.cpp ${SRCS})


# tcp_client test
add_executable(example_client tests/example_client.cpp ${SRCS})

# free_server_client test
add_executable(free_server_client tests/FreeServerClient_test.cpp ${SRCS})

# ping-pong
add_executable(pingpong_server examples/ping-pong/server.cpp ${SRCS})
add_executable(pingpong_client examples/ping-pong/client.cpp ${SRCS})

# buffer test
add_executable(buffer_test tests/buffer_test.cpp ${SRCS})