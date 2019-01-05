cmake_minimum_required(VERSION 3.0)
project(evio-4.4.6)

#include_directories($ENV{CODA}/common/include)
include_directories(src/libsrc src/libsrc++)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

message("--------------- evio-4.4.6 -------------------------")
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
  src/libsrc/eviofmtswap.c
  src/libsrc/evioswap.c
  )
#Generate the shared library from the sources
add_library(evio-4.4.6 SHARED ${SOURCE_FILES_EVIO})

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

add_library(eviocxx-4.4.6 SHARED ${SOURCE_FILES_EVIOCXX})
add_dependencies(eviocxx-4.4.6 evio-4.4.6)
target_link_libraries(eviocxx-4.4.6 evio-4.4.6 expat z )

add_executable(evio2xml src/execsrc/evio2xml.cc)
add_dependencies(evio2xml eviocxx-4.4.6)
target_link_libraries(evio2xml eviocxx-4.4.6 expat z)

add_executable(eviocopy src/execsrc/eviocopy.c)
add_dependencies(eviocopy evio-4.4.6)
target_link_libraries(eviocopy evio-4.4.6)

install(TARGETS evio-4.4.6 eviocxx-4.4.6      DESTINATION lib)
install(TARGETS evio2xml eviocopy         DESTINATION bin)

