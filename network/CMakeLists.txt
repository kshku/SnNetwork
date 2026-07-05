if(SN_NETWORK_BUILD_SHARED)
    add_library(snnetwork SHARED)
    target_compile_definitions(snnetwork PRIVATE SN_EXPORT)
else()
    add_library(snnetwork STATIC)
    target_compile_definitions(snnetwork PUBLIC SN_NETWORK_STATIC)
endif()

target_include_directories(snnetwork PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(snnetwork PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(snnetwork PRIVATE sn_network_configs)
target_link_libraries(snnetwork PUBLIC sncore)

add_subdirectory(src)
