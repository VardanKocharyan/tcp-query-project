add_executable(test_message
    unit/test_message.c
)

target_link_libraries(test_message
    PRIVATE
        tcp_query_core
)

add_test(
    NAME test_message
    COMMAND test_message
)


add_executable(test_query_handler
    unit/test_query_handler.c
)

target_link_libraries(test_query_handler
    PRIVATE
        tcp_query_core
)

add_test(
    NAME test_query_handler
    COMMAND test_query_handler
)

add_executable(test_codec
    unit/test_codec.c
)

target_link_libraries( test_codec
    PRIVATE
        tcp_query_core
)

add_test(
    NAME test_codec
    COMMAND test_codec
)

add_executable( test_serializer
    unit/test_serializer.c
    ../src/protocol/serializer.c
    
)
target_link_libraries( test_serializer
    PRIVATE
        tcp_query_core

)
add_test(
    NAME test_serializer
    COMMAND test_serializer
)

add_executable( test_deserializer
    unit/test_deserializer.c
    ../src/protocol/serializer.c
    ../src/protocol/deserializer.c
)
target_link_libraries( test_deserializer
    PRIVATE
        tcp_query_core
)
add_test(
    NAME test_deserializer
    COMMAND test_deserializer
)

add_executable( test_response_serializer
    unit/test_response_serializer.c
    ../src/protocol/response_serializer.c
)
target_link_libraries( test_response_serializer
    PRIVATE
        tcp_query_core
)
add_test(
    NAME test_response_serializer
    COMMAND test_response_serializer
)

add_executable( test_response_deserializer
    unit/test_response_deserializer.c
    ../src/protocol/response_serializer.c
    ../src/protocol/response_deserializer.c
)
target_link_libraries( test_response_deserializer
    PRIVATE
        tcp_query_core
)
add_test(
    NAME test_response_deserializer
    COMMAND test_response_deserializer
)

add_executable( test_framer
    unit/test_framer.c
    ../src/protocol/framer.c
)
target_link_libraries( test_framer
    PRIVATE
        tcp_query_core
)
add_test(
    NAME test_framer
    COMMAND test_framer
)

add_executable( test_tcp
    unit/test_tcp.c
    ../src/network/tcp.c
)
target_link_libraries( test_tcp
    PRIVATE
        tcp_query_core
)
add_test(
    NAME test_tcp
    COMMAND test_tcp
)

add_executable(test_tcp_integration
    integration/test_tcp_integration.c
)

add_dependencies(test_tcp_integration tcp-query-server)

target_compile_definitions(test_tcp_integration
    PRIVATE
        SERVER_PATH=\"$<TARGET_FILE:tcp-query-server>\"
)

target_link_libraries(test_tcp_integration PRIVATE tcp_query_core)

add_test(
    NAME test_tcp_integration
    COMMAND test_tcp_integration
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

set_tests_properties(
    test_tcp_integration
    PROPERTIES
        TIMEOUT 10
)
