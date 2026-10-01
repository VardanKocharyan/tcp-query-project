cmake_minimum_required(VERSION 3.15)

project(tcp-query-project
    VERSION 1.0.0
    LANGUAGES C
)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

message(STATUS "--------------------------------------------------------")
message(STATUS "This project requires standard C17, and compiler-specific C extensions are not allowed.")
message(STATUS "--------------------------------------------------------")

enable_testing()

# Core library
add_library(tcp_query_core STATIC
    src/core/query_handler.c
    src/protocol/message.c
    src/protocol/codec.c
)

target_include_directories(tcp_query_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(tcp_query_core PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Werror
    )
elseif(MSVC)
    target_compile_options(tcp_query_core PRIVATE
        /W4
        /WX
    )
endif()

# Server
add_executable(tcp-query-server
    app/server_main.c
)

target_link_libraries(tcp-query-server
    PRIVATE
        tcp_query_core
)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(tcp-query-server PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Werror
    )
elseif(MSVC)
    target_compile_options(tcp-query-server PRIVATE
        /W4
        /WX
    )
endif()

# Client
add_executable(tcp-query-client
    app/client_main.c
)

target_link_libraries(tcp-query-client
    PRIVATE
        tcp_query_core
)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(tcp-query-client PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Werror
    )
elseif(MSVC)
    target_compile_options(tcp-query-client PRIVATE
        /W4
        /WX
    )
endif()



# Tests
add_subdirectory(tests)

