cmake_minimum_required(VERSION 3.16)

project(fn_antlr_demo VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 优先使用编译和链接阶段都生效的 -pthread，而不只是链接 -lpthread。
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Demo 与正式项目共用仓库根目录中固定为 4.13.2 的官方 Runtime 源码。
get_filename_component(FN_REPOSITORY_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
set(ANTLR4_RUNTIME_DIR
    "${FN_REPOSITORY_ROOT}/third_party/antlr4-runtime/runtime/src")

file(GLOB_RECURSE ANTLR4_RUNTIME_SOURCES CONFIGURE_DEPENDS
    "${ANTLR4_RUNTIME_DIR}/*.cpp")

add_library(antlr4_runtime STATIC ${ANTLR4_RUNTIME_SOURCES})
target_include_directories(antlr4_runtime PUBLIC "${ANTLR4_RUNTIME_DIR}")
target_link_libraries(antlr4_runtime PUBLIC Threads::Threads)

if(WIN32)
    target_compile_definitions(antlr4_runtime PUBLIC ANTLR4CPP_STATIC)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_compile_options(antlr4_runtime PUBLIC -pthread)
        target_link_libraries(antlr4_runtime PUBLIC -pthread)
    endif()
    # 这些是 Runtime 上游源码自身会触发、但不影响本 Demo 的警告。
    target_compile_options(antlr4_runtime PRIVATE
        -Wno-deprecated-declarations
        -Wno-overloaded-virtual
        -Wno-multichar)
endif()

set(FNSQL_GENERATED_SOURCES
    generated/FNSqlLexer.cpp
    generated/FNSqlParser.cpp)

add_executable(fn_demo
    src/main.cpp
    ${FNSQL_GENERATED_SOURCES})

target_include_directories(fn_demo PRIVATE generated)
target_link_libraries(fn_demo PRIVATE antlr4_runtime)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(fn_demo PRIVATE -Wall -Wextra -Wpedantic)
endif()
