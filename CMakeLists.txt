cmake_minimum_required(VERSION 3.16)

project(fn_mysql_proxy VERSION 0.1.0 LANGUAGES CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Boost REQUIRED COMPONENTS system)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# ANTLR Generator 和 Runtime 都固定为 4.13.2。正式项目与 docs 中的
# 学习 Demo 共用这一份官方 Runtime 源码。
set(ANTLR4_RUNTIME_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/antlr4-runtime/runtime/src")

file(GLOB_RECURSE ANTLR4_RUNTIME_SOURCES CONFIGURE_DEPENDS
    "${ANTLR4_RUNTIME_DIR}/*.cpp")

add_library(antlr4_runtime STATIC ${ANTLR4_RUNTIME_SOURCES})
target_include_directories(antlr4_runtime PUBLIC "${ANTLR4_RUNTIME_DIR}")
target_link_libraries(antlr4_runtime PUBLIC Threads::Threads)

if(WIN32)
    target_compile_definitions(antlr4_runtime PUBLIC ANTLR4CPP_STATIC)
endif()

add_library(fn_protocol
    src/protocol/mysql_packet_codec.cpp
)

target_include_directories(fn_protocol PUBLIC include)

add_library(fn_sql
    src/sql/parser.cpp
    generated/FNCommandLexer.cpp
    generated/FNCommandParser.cpp
)

target_include_directories(fn_sql PUBLIC include)
target_include_directories(fn_sql PRIVATE generated)
target_link_libraries(fn_sql PRIVATE antlr4_runtime)

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
        fn_protocol
        fn_sql
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        # 即使旧 build 目录缓存过 -lpthread，也保证 std::call_once 的编译单元
        # 使用 -pthread；该选项通过 PUBLIC 传给生成的 Lexer / Parser。
        target_compile_options(antlr4_runtime PUBLIC -pthread)
        target_link_libraries(antlr4_runtime PUBLIC -pthread)
    endif()
    target_compile_options(antlr4_runtime PRIVATE
        -Wno-deprecated-declarations
        -Wno-overloaded-virtual
        -Wno-multichar)
    target_compile_options(fn_protocol PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(fn_sql PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(fn_proxy PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(BUILD_TESTING)
    add_executable(fn_unit_tests
        tests/unit_tests.cpp
    )

    target_link_libraries(fn_unit_tests PRIVATE fn_protocol fn_sql)

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(fn_unit_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME fn_unit_tests COMMAND fn_unit_tests)

    find_package(Python3 QUIET COMPONENTS Interpreter)
    if(Python3_Interpreter_FOUND)
        add_test(
            NAME fn_proxy_smoke
            COMMAND
                ${CMAKE_COMMAND} -E env
                "FN_PROXY_TEST_BINARY=$<TARGET_FILE:fn_proxy>"
                ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/smoke_fake_mysql.py
        )
        set_tests_properties(fn_proxy_smoke PROPERTIES TIMEOUT 15)
    endif()
endif()
