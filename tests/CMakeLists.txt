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
