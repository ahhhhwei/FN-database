cmake_minimum_required(VERSION 3.16)

project(fn_mysql_proxy VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Boost REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)

add_executable(fn_proxy
    src/main.cpp
    src/config.cpp
    src/proxy_server.cpp
    src/proxy_session.cpp
)

target_include_directories(fn_proxy PRIVATE include)

target_link_libraries(fn_proxy
    PRIVATE
        Boost::system
        Threads::Threads
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(fn_proxy PRIVATE -Wall -Wextra -Wpedantic)
endif()
