enable_testing()

set(TEST_COMMON_SRCS)

add_executable(sn_network_test)
target_sources(sn_network_test PRIVATE test.c ${TEST_COMMON_SRCS})
target_link_libraries(sn_network_test PRIVATE snnetwork)
add_test(NAME sn_network_test COMMAND sn_network_test)

add_executable(sn_network_tcp_test)
target_sources(sn_network_tcp_test PRIVATE tcp_test.c ${TEST_COMMON_SRCS})
target_link_libraries(sn_network_tcp_test PRIVATE snnetwork)
add_test(NAME sn_network_tcp_test COMMAND sn_network_tcp_test)

add_executable(sn_network_udp_test)
target_sources(sn_network_udp_test PRIVATE udp_test.c ${TEST_COMMON_SRCS})
target_link_libraries(sn_network_udp_test PRIVATE snnetwork)
add_test(NAME sn_network_udp_test COMMAND sn_network_udp_test)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows" AND SN_NETWORK_BUILD_SHARED)
    add_custom_target(copy_dlls ALL
        COMMENT "Copy the dlls"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_RUNTIME_DLLS:sn_network_test>" "$<TARGET_FILE_DIR:sn_network_test>"
        COMMAND_EXPAND_LISTS
    )

    add_dependencies(copy_dlls sn_network_test)
endif()

