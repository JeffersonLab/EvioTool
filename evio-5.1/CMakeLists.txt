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
add_library(evio-5.1       SHARED ${SOURCE_FILES_EVIO})
add_library(evio-5.1static STATIC ${SOURCE_FILES_EVIO})

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
target_link_libraries(eviocxx-5.1 evio-5.1 expat z pthread)

add_executable(evio2xml src/execsrc/evio2xml.c)
add_dependencies(evio2xml evio-5.1static)
target_link_libraries(evio2xml evio-5.1static expat z pthread)

add_executable(eviocopy src/execsrc/eviocopy.c)
add_dependencies(eviocopy evio-5.1static)
target_link_libraries(eviocopy evio-5.1static pthread)

add_executable(evioconcat src/execsrc/evioconcat.cc)
add_dependencies(evioconcat evio-5.1static )
target_link_libraries(evioconcat evio-5.1static pthread)

add_executable(eviosplit src/execsrc/eviosplit.cc)
add_dependencies(eviosplit evio-5.1static)
target_link_libraries(eviosplit evio-5.1static pthread)

install(TARGETS evio-5.1 eviocxx-5.1      DESTINATION lib)
install(TARGETS evio2xml eviocopy evioconcat eviosplit    DESTINATION bin)
