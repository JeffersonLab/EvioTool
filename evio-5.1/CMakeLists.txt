cmake_minimum_required(VERSION 3.0)
project(evio-5.1)

#include_directories($ENV{CODA}/common/include)
include_directories(src/libsrc src/libsrc++)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

message("--------------- evio-5.1 -------------------------")
message("We have CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")
# message("CMAKE_C_FLAGS_RELEASE    = ${CMAKE_C_FLAGS_RELEASE}")
# message("CMAKE_C_FLAGS_DEBUG      = ${CMAKE_C_FLAGS_DEBUG}")
# message("CMAKE_C_FLAGS            = ${CMAKE_C_FLAGS}")
message("CMAKE_CXX_FLAGS          = ${CMAKE_CXX_FLAGS}")
# message("CMAKE_CXX_FLAGS_RELEASE  = ${CMAKE_CXX_FLAGS_RELEASE}")
# message("CMAKE_CXX_FLAGS_DEBUG    = ${CMAKE_CXX_FLAGS_DEBUG}")
# message("---------------------------------------------")

set(SOURCE_FILES_EVIO
  src/libsrc/evio.h
  src/libsrc/msinttypes.h
  src/libsrc/evio.c
  src/libsrc/eviofmt.c
  src/libsrc/eviofmtdump.c
  src/libsrc/eviofmtswap.c
  src/libsrc/evioswap.c
  )
#Generate the shared library from the sources
add_library(evio-5.1 SHARED ${SOURCE_FILES_EVIO})

set(SOURCE_FILES_EVIOCXX
  src/libsrc++/evioBankIndex.hxx
  src/libsrc++/evioBufferChannel.hxx
  src/libsrc++/evioFileChannel.hxx
  src/libsrc++/evioSocketChannel.hxx
  src/libsrc++/evioChannel.hxx
  src/libsrc++/evioDictionary.hxx
  src/libsrc++/evioException.hxx
  src/libsrc++/evioTypedefs.hxx
  src/libsrc++/evioUtil.hxx
  src/libsrc++/evioUtilTemplates.hxx
  src/libsrc++/evioBankIndex.cc
  src/libsrc++/evioBufferChannel.cc
  src/libsrc++/evioFileChannel.cc
  src/libsrc++/evioSocketChannel.cc
  src/libsrc++/evioDictionary.cc
  src/libsrc++/evioException.cc
  src/libsrc++/evioUtil.cc
  )

add_library(eviocxx-5.1 SHARED ${SOURCE_FILES_EVIOCXX})
add_dependencies(eviocxx-5.1 evio-5.1)
target_link_libraries(eviocxx-5.1 evio-5.1 expat z )

add_executable(evio2xml src/execsrc/evio2xml.c)
add_dependencies(evio2xml evio-5.1)
target_link_libraries(evio2xml evio-5.1 expat z)

add_executable(eviocopy src/execsrc/eviocopy.c)
add_dependencies(eviocopy evio-5.1)
target_link_libraries(eviocopy evio-5.1)

install(TARGETS evio-5.1 eviocxx-5.1      DESTINATION lib)
install(TARGETS evio2xml eviocopy         DESTINATION bin)

  # src/execsrc/evio2xml.c
  # src/execsrc/evioCat.cc
  # src/execsrc/eviocopy.c
  
  #   src/examples/dana.cc
  #   src/examples/etst1.cc
  #   src/examples/etst2.cc
  #   src/examples/etst3.cc
  #   src/examples/etst4.cc
  #   src/examples/etst5.cc
  #   src/examples/etst6.cc
  #   src/examples/etst7.cc
  #   src/examples/etst8.cc
  #   src/examples/etst9.cc
  #   src/examples/etst10.cc
  #   src/examples/etst11.cc
  #   src/examples/etst12.cc
  #   src/examples/etst13.cc
  #   src/examples/etst14.cc
  #   src/examples/etst15.cc
  #   src/examples/etst16.cc
  #   src/examples/etst17.cc

  #   src/test/elliottTest.cc
  #   src/test/evMultithread.c
  #   src/test/evReadPipe.c
  #   src/test/evtest.c
  #   src/test/evtest2.c
  #   src/test/evtest3.c
  #   src/test/evtestBuf.c
  #   src/test/evtestBuf2.c
  #   src/test/evtestAppend.c
  #   src/test/evTestHandles.c
  #   src/test/evTestFirstEvent.c
  #   src/test/evtestRead.c
  #   src/test/evtestRio.c
  #   src/test/evtestSock.c
  #   src/test/evWritePipe.c
  #   src/test/splitTest.c
  #   src/test/stringTest.c)


# add_executable(evio_5_0 ${SOURCE_FILES} src/execsrc/evio2xml.c src/execsrc/eviocopy.c src/execsrc/evioCat.cc )