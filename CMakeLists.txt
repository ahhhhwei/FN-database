cmake_minimum_required(VERSION 3.16)

project(fn_mysql_proxy VERSION 0.1.0 LANGUAGES CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Boost REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)

add_library(fn_protocol
    src/protocol/mysql_packet_codec.cpp
)

target_include_directories(fn_protocol PUBLIC include)

add_library(fn_sql
    src/sql/parser.cpp
)

target_include_directories(fn_sql PUBLIC include)

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
endif()
