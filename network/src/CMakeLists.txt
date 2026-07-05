set(HEADERFILES
    api.h
    network.h
)

set(SRCS
    address.c
)

set(SPECIFIC_SRCS
    network.c
    tcp.c
    udp.c
)

set(SUBDIRS
    nix
    win32
)

set(INCLUDE_BASE "${PROJECT_SOURCE_DIR}/network/include/snnetwork")

list(TRANSFORM HEADERFILES PREPEND "${INCLUDE_BASE}/")

target_sources(snnetwork PRIVATE ${HEADERFILES} ${SRCS})

foreach(dir IN LISTS SUBDIRS)
    foreach(src IN LISTS SPECIFIC_SRCS)
        target_sources(snnetwork PRIVATE "${dir}/${src}")
    endforeach()
endforeach()
