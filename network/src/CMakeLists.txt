set(HEADERFILES
    api.h
    network.h
)

set(SRCS
    network.c
)

set(SPECIFIC_SRCS
)

set(SUBDIRS
    win32
    pthread
)

set(INCLUDE_BASE "${PROJECT_SOURCE_DIR}/network/include/snnetwork")

list(TRANSFORM HEADERFILES PREPEND "${INCLUDE_BASE}/")

target_sources(snnetwork PRIVATE ${HEADERFILES} ${SRCS})

foreach(dir IN LISTS SUBDIRS)
    foreach(src IN LISTS SPECIFIC_SRCS)
        target_sources(snnetwork PRIVATE "${dir}/${src}")
    endforeach()
endforeach()
