# 要求 CMake 3.16+，这样下面使用的 target_*、CTest 和 FindPython3 行为稳定。
cmake_minimum_required(VERSION 3.16)

# 声明项目名称、版本和使用的编程语言；这里只启用 C++，可减少无关探测。
project(fn_mysql_proxy VERSION 0.1.0 LANGUAGES CXX)

# include(CTest) 创建 BUILD_TESTING 选项，并提供 add_test/set_tests_properties。
include(CTest)

# 所有自有目标统一使用标准 C++17，不允许退回编译器私有的 gnu++17 方言。
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Boost.System 提供 Asio 使用的 error_code 支持；Threads 提供跨平台线程链接参数。
find_package(Boost REQUIRED COMPONENTS system)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# ANTLR Generator 和 Runtime 都固定为 4.13.2。正式项目与 docs 中的
# 学习 Demo 共用这一份官方 Runtime 源码。
set(ANTLR4_RUNTIME_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/antlr4-runtime/runtime/src")

# CONFIGURE_DEPENDS 使 Runtime 新增/删除 cpp 时触发 CMake 重新配置。
file(GLOB_RECURSE ANTLR4_RUNTIME_SOURCES CONFIGURE_DEPENDS
    "${ANTLR4_RUNTIME_DIR}/*.cpp")

# 将 ANTLR Runtime 单独做成静态库，避免每个使用 Parser 的目标重复编译源码。
add_library(antlr4_runtime STATIC ${ANTLR4_RUNTIME_SOURCES})
# PUBLIC 表示 include 路径和线程依赖也会传递给链接 antlr4_runtime 的目标。
target_include_directories(antlr4_runtime PUBLIC "${ANTLR4_RUNTIME_DIR}")
target_link_libraries(antlr4_runtime PUBLIC Threads::Threads)

# Windows 构建静态 ANTLR Runtime 时需要该宏控制导入/导出声明。
if(WIN32)
    target_compile_definitions(antlr4_runtime PUBLIC ANTLR4CPP_STATIC)
endif()

# fn_protocol 不依赖网络和 ANTLR，只负责 MySQL Classic Protocol 编解码。
add_library(fn_protocol
    src/protocol/mysql_packet_codec.cpp
)

# 使用 PUBLIC 是因为该库的公共头文件也位于 include/ 下。
target_include_directories(fn_protocol PUBLIC include)

# fn_sql 将手写适配层与 ANTLR 根据 FNCommand.g4 生成的源码编译到一起。
add_library(fn_sql
    src/sql/parser.cpp
    generated/FNCommandLexer.cpp
    generated/FNCommandParser.cpp
)

target_include_directories(fn_sql PUBLIC include)
# generated 只出现在 fn_sql 的实现中，调用者不应直接依赖生成头文件。
target_include_directories(fn_sql PRIVATE generated)
target_link_libraries(fn_sql PRIVATE antlr4_runtime)

# 可执行程序负责配置、监听、会话和网络转发；协议/SQL 逻辑通过库链接进来。
add_executable(fn_proxy
    src/main.cpp
    src/config.cpp
    src/proxy_server.cpp
    src/proxy_session.cpp
)

target_include_directories(fn_proxy PRIVATE include)

# PRIVATE 表示这些库只用于构建 fn_proxy，不作为可传递接口发布。
target_link_libraries(fn_proxy
    PRIVATE
        Boost::system
        Threads::Threads
        fn_protocol
        fn_sql
)

# 自有代码启用常用警告；ANTLR 上游源码的一些已知警告单独关闭，避免淹没输出。
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
    # 单元测试直接链接两个纯逻辑库，不需要启动真实 MySQL。
    add_executable(fn_unit_tests
        tests/unit_tests.cpp
    )

    target_link_libraries(fn_unit_tests PRIVATE fn_protocol fn_sql)

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(fn_unit_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME fn_unit_tests COMMAND fn_unit_tests)

    # Python 只用于端到端冒烟测试；系统没有解释器时仍允许构建正式程序。
    find_package(Python3 QUIET COMPONENTS Interpreter)
    if(Python3_Interpreter_FOUND)
        add_test(
            NAME fn_proxy_smoke
            COMMAND
                # 通过环境变量把本次构建生成的准确可执行文件路径传给脚本。
                # $<TARGET_FILE:...> 是在生成构建系统时求值的 generator expression。
                ${CMAKE_COMMAND} -E env
                "FN_PROXY_TEST_BINARY=$<TARGET_FILE:fn_proxy>"
                ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/smoke_fake_mysql.py
        )
        # 防止网络状态机出错时测试永久等待。
        set_tests_properties(fn_proxy_smoke PROPERTIES TIMEOUT 15)
    endif()
endif()
