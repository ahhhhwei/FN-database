cmake_minimum_required(VERSION 3.16)
project(fn_mysql_proxy_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(fn_proxy src/main.cpp)
target_compile_options(fn_proxy PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(fn_proxy PRIVATE pthread)
