enable_testing()

set(SRCS
    test.c
)

add_executable(sn_network_test)
target_sources(sn_network_test PRIVATE ${SRCS})
target_link_libraries(sn_network_test PRIVATE snnetwork)

add_test(NAME sn_network_test COMMAND sn_network_test)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows" AND SN_NETWORK_BUILD_SHARED)
    add_custom_target(copy_dlls ALL
        COMMENT "Copy the dlls"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_RUNTIME_DLLS:sn_network_test>" "$<TARGET_FILE_DIR:sn_network_test>"
        COMMAND_EXPAND_LISTS
    )

    add_dependencies(copy_dlls sn_network_test)
endif()

