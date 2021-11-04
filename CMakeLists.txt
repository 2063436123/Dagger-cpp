cmake_minimum_required(VERSION 3.16)
project(testLinux)

set(CMAKE_CXX_STANDARD 20)

link_libraries(pthread gtest)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
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
add_executable(free_server_client tests/FreeServerClient.cpp ${SRCS})

# ping-pong test
add_executable(pingpong_server examples/ping-pong/server.cpp ${SRCS})
add_executable(pingpong_client examples/ping-pong/client.cpp ${SRCS})
add_executable(plain_server examples/ping-pong/plain_server.cpp ${SRCS})

# buffer test
add_executable(buffer_test tests/buffer_test.cpp ${SRCS})

# logger test
add_executable(logger_test tests/logger_test.cpp ${SRCS})

# inAddr test
add_executable(inAddr_test tests/inAddr_test.cpp ${SRCS})

# spinLock test
add_executable(spinLock_test tests/spinLock_test.cpp)

# ssl
add_executable(ssl_client examples/secure-socket/ssl_client.cpp ${SRCS})
target_link_libraries(ssl_client PUBLIC ssl crypto)
add_executable(ssl_server examples/secure-socket/ssl_server.cpp ${SRCS})
target_link_libraries(ssl_server PUBLIC ssl crypto)

# asio
add_executable(asio_server examples/asio/asio_server.cpp)
add_executable(asio_multi_server examples/asio/asio_multi_server.cpp)

# http
add_executable(http_server tests/simple_http.cpp ${SRCS})