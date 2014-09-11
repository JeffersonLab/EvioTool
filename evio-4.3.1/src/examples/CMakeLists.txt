include_directories("${PROJECT_SOURCE_DIR}/src/libsrc")
include_directories("${PROJECT_SOURCE_DIR}/src/libsrc++")

set(EVIO_EXAMPLES
    dana
    etst1
    etst2
    etst3
    etst4
    etst5
    etst6
    etst7
    etst8
    etst9
    etst10
    etst11
    etst12
    etst13
    etst14
    etst15
    etst16
    etst17
)

foreach(file ${EVIO_EXAMPLES})
  add_executable(${file} ${file}.cc)
  target_link_libraries(${file} evioxx evio)
endforeach()

if (EVIO_INSTALL_EXAMPLES)
  install(TARGETS ${EVIO_EXAMPLES} DESTINATION ${CMAKE_INSTALL_BINDIR})
endif (EVIO_INSTALL_EXAMPLES)
