find_package(Threads REQUIRED)

set(EVIO_SRC
    evio.c
    evioswap.c
    eviofmt.c
    eviofmtswap.c
)

set(EVIO_H
    evio.h
)

add_library(evio SHARED ${EVIO_SRC})
target_link_libraries(evio ${CMAKE_THREAD_LIBS_INIT})

install(FILES ${EVIO_H} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS evio DESTINATION ${CMAKE_INSTALL_LIBDIR})
