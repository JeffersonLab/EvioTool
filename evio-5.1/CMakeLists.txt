cmake_minimum_required(VERSION 3.0)
project(evio VERSION 5.1 LANGUAGES C CXX)

# Set a default build type if none was specified
set(default_build_type "RELEASE")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
                 "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Offer the user the choice of overriding the installation directories
set(INSTALL_LIB_DIR lib CACHE PATH "Installation directory for libraries")
set(INSTALL_BIN_DIR bin CACHE PATH "Installation directory for executables")
set(INSTALL_INCLUDE_DIR include CACHE PATH  "Installation directory for header files")
set(INSTALL_CMAKE_DIR lib/cmake/evio)

# Make relative paths absolute (needed later on)
foreach(p LIB BIN INCLUDE CMAKE)
    set(var INSTALL_${p}_DIR)
    if(NOT IS_ABSOLUTE "${${var}}")
        set(${var} "${CMAKE_INSTALL_PREFIX}/${${var}}")
    endif()
endforeach()


set(INCLUDE_INSTALL_DIR ${INSTALL_INCLUDE_DIR}/evio)
set(LIB_INSTALL_DIR ${INSTALL_LIB_DIR})

#include_directories($ENV{CODA}/common/include)
include_directories(src/libsrc src/libsrc++)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

message("--------------- ${EVIO_LIBRARY} -------------------------")
message("We have CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")
# message("CMAKE_C_FLAGS_RELEASE    = ${CMAKE_C_FLAGS_RELEASE}")
# message("CMAKE_C_FLAGS_DEBUG      = ${CMAKE_C_FLAGS_DEBUG}")
# message("CMAKE_C_FLAGS            = ${CMAKE_C_FLAGS}")
message("CMAKE_CXX_FLAGS            = ${CMAKE_CXX_FLAGS}")
# message("CMAKE_CXX_FLAGS_RELEASE  = ${CMAKE_CXX_FLAGS_RELEASE}")
# message("CMAKE_CXX_FLAGS_DEBUG    = ${CMAKE_CXX_FLAGS_DEBUG}")
message("INCLUDE_INSTALL_DIR        = ${INCLUDE_INSTALL_DIR}")
message("LIB_INSTALL_DIR            = ${LIB_INSTALL_DIR}")
message("INSTALL_CMAKE_DIR          = ${INSTALL_CMAKE_DIR}")
message("--------------------------------------------------")

set(SOURCE_FILES_EVIO
    src/libsrc/msinttypes.h
    src/libsrc/evio.c
    src/libsrc/eviofmt.c
    src/libsrc/eviofmtdump.c
    src/libsrc/eviofmtswap.c
    src/libsrc/evioswap.c
  )

set(HEADER_FILES_EVIO
    src/libsrc/evio.h
    )

#Generate the shared library from the sources
add_library(evio       SHARED ${SOURCE_FILES_EVIO})
add_library(eviostatic STATIC ${SOURCE_FILES_EVIO})

target_include_directories(evio
                           PUBLIC
                           $<INSTALL_INTERFACE:include/evio>
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/libsrc>
                           )

install(TARGETS evio
        EXPORT  evio-export
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

install(EXPORT evio-export
        FILE evioTargets.cmake
        DESTINATION ${INSTALL_CMAKE_DIR})

install(FILES ${HEADER_FILES_EVIO}
        DESTINATION ${INCLUDE_INSTALL_DIR})

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/evioConfig.cmake
        DESTINATION ${INSTALL_CMAKE_DIR})
#
# Now deal with some of the useful executables in the package.
#

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


# add_library(eviocxx-5.1 SHARED ${SOURCE_FILES_EVIOCXX})
# add_dependencies(eviocxx-5.1 evio-5.1)
# target_link_libraries(eviocxx-5.1 evio-5.1 expat z pthread)

add_executable(evio2xml src/execsrc/evio2xml.c)
add_dependencies(evio2xml eviostatic)
target_link_libraries(evio2xml eviostatic expat z pthread)

add_executable(eviocopy src/execsrc/eviocopy.c)
add_dependencies(eviocopy eviostatic)
target_link_libraries(eviocopy eviostatic pthread)

add_executable(evioconcat src/execsrc/evioconcat.cc)
add_dependencies(evioconcat eviostatic )
target_link_libraries(evioconcat eviostatic pthread)

add_executable(eviosplit src/execsrc/eviosplit.cc)
add_dependencies(eviosplit eviostatic)
target_link_libraries(eviosplit eviostatic pthread)

install(TARGETS evio2xml eviocopy evioconcat eviosplit    DESTINATION bin)

