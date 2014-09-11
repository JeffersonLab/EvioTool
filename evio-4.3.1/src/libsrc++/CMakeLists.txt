include_directories("${PROJECT_SOURCE_DIR}/src/libsrc")

find_package(EXPAT REQUIRED)

set(EVIOXX_SRC
    evioException.cc
    evioDictionary.cc
    evioFileChannel.cc
    evioBufferChannel.cc
    evioSocketChannel.cc
    evioBankIndex.cc
    evioUtil.cc
)

set(EVIOXX_H
    evioException.hxx
    evioTypedefs.hxx
    evioDictionary.hxx
    evioChannel.hxx
    evioFileChannel.hxx
    evioBufferChannel.hxx
    evioSocketChannel.hxx
    evioBankIndex.hxx
    evioUtil.hxx
    evioUtilTemplates.hxx
)

add_library(evioxx SHARED ${EVIOXX_SRC})
target_link_libraries(evioxx evio ${EXPAT_LIBRARIES})

install(FILES ${EVIOXX_H} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS evioxx DESTINATION ${CMAKE_INSTALL_LIBDIR})
