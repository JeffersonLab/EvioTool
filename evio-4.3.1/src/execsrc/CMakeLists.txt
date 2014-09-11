include_directories("${PROJECT_SOURCE_DIR}/src/libsrc")
include_directories("${PROJECT_SOURCE_DIR}/src/libsrc++")

find_package(EXPAT REQUIRED)
find_package(ZLIB REQUIRED)

set(EXEC_C
    eviocopy
    xml2evio
)

add_executable(eviocopy eviocopy.c)
target_link_libraries(eviocopy evio)

add_executable(xml2evio xml2evio.c)
target_link_libraries(xml2evio evio ${EXPAT_LIBRARIES} ${ZLIB_LIBRARIES})


set(EXEC_CXX
    evio2xml
    evioCat
)

foreach(file ${EXEC_CXX})
  add_executable(${file} ${file}.cc)
  target_link_libraries(${file} evioxx)
endforeach()

install(TARGETS ${EXEC_C} ${EXEC_CXX} DESTINATION ${CMAKE_INSTALL_BINDIR})
