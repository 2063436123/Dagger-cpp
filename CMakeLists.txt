cmake_minimum_required(VERSION 3.16)
project(testLinux)

set(CMAKE_CXX_STANDARD 20)

# tcp_server test
add_executable(example_server tests/example_server.cpp src/Sources/Socket.cpp
        src/Sources/Epoller.cpp src/Sources/TcpConnection.cpp
        src/Sources/EventLoop.cpp
        src/Sources/Logger.cpp
        third_party/fmt_src/os.cc third_party/fmt_src/format.cc
        src/Sources/TcpServer.cpp)

target_include_directories(example_server PUBLIC third_party/fmt_include)
target_link_libraries(example_server pthread)
#tcp_client test
add_executable(example_client tests/example_client.cpp src/Sources/Socket.cpp
        src/Sources/Epoller.cpp src/Sources/TcpConnection.cpp
        src/Sources/EventLoop.cpp
        src/Sources/Logger.cpp
        third_party/fmt_src/os.cc third_party/fmt_src/format.cc
        )
target_include_directories(example_client PUBLIC third_party/fmt_include)
target_link_libraries(example_client pthread)
#free_server_client test
add_executable(free_server_client tests/FreeServerClient_test.cpp src/Sources/Socket.cpp
        src/Sources/Epoller.cpp src/Sources/TcpConnection.cpp
        src/Sources/EventLoop.cpp
        src/Sources/Logger.cpp
        third_party/fmt_src/os.cc third_party/fmt_src/format.cc)
target_include_directories(free_server_client PUBLIC third_party/fmt_include)
target_link_libraries(free_server_client pthread)