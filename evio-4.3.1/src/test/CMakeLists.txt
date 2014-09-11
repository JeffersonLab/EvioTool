include_directories("${PROJECT_SOURCE_DIR}/src/libsrc")
include_directories("${PROJECT_SOURCE_DIR}/src/libsrc++")

find_package(Threads REQUIRED)

set(TEST_C_PTHREAD
    evMultithread
    evtestSock
)

set(TEST_C
    evtest2
    evtest3
    evtestBuf
    evtest
    evtestRead
    evtestRio
)

set(TEST_CXX
    elliottTest
)

foreach(file ${TEST_C_PTHREAD})
  add_executable(${file} ${file}.c)
  target_link_libraries(${file} evio ${CMAKE_THREAD_LIBS_INIT})
endforeach()

foreach(file ${TEST_C})
  add_executable(${file} ${file}.c)
  target_link_libraries(${file} evio)
endforeach()

foreach(file ${TEST_CXX})
  add_executable(${file} ${file}.cc)
  target_link_libraries(${file} evioxx evio)
endforeach()
