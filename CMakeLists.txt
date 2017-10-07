cmake_minimum_required(VERSION 3.2)
project(proxy_server)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread -Wall -Wextra")

set(SOURCE_FILES main.cpp proxy/request_processing/header_parser.cpp
        proxy/request_processing/header_parser.h proxy/proxy_server.cpp proxy/proxy_server.h proxy/request_processing/resolver.h proxy/util/util.cpp
        proxy/request_processing/buffered_message.h proxy/request_processing/buffered_message.cpp proxy/util/file_descriptor.cpp proxy/util/file_descriptor.h proxy/epoll_queue/timer_fd.cpp proxy/epoll_queue/timer_fd.h proxy/util/event_fd.cpp proxy/util/event_fd.h proxy/util/signal_fd.cpp proxy/util/signal_fd.h proxy/util/socket_wrap.cpp proxy/util/socket_wrap.h proxy/epoll_queue/fd_state.cpp proxy/epoll_queue/fd_state.h proxy/epoll_queue/epoll_core.cpp proxy/epoll_queue/epoll_core.h proxy/epoll_queue/epoll_elem.cpp proxy/epoll_queue/epoll_elem.h proxy/util/annotated_exception.cpp proxy/util/annotated_exception.h proxy/request_processing/simple_cache.cpp proxy/request_processing/simple_cache.h proxy/epoll_queue/connection.cpp proxy/epoll_queue/connection.h proxy/epoll_queue/epoll_queue.cpp proxy/epoll_queue/epoll_queue.h proxy/request_processing/resolver.cpp)

add_executable(proxy_server ${SOURCE_FILES})
