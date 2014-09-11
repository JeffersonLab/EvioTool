project(evio CXX C)
cmake_minimum_required(VERSION 2.6)

if (NOT CMAKE_BUILD_TYPE)
  message(STATUS "No build type selected, default to Release")
  set(CMAKE_BUILD_TYPE "Release")
endif()


if(NOT CMAKE_INSTALL_BINDIR)
  set(CMAKE_INSTALL_BINDIR "bin" CACHE PATH "user executables (bin)")
endif()

if(NOT CMAKE_INSTALL_LIBDIR)
  set(CMAKE_INSTALL_LIBDIR "lib" CACHE PATH "object code libraries (lib)")
endif()

if(NOT CMAKE_INSTALL_INCLUDEDIR)
  set(CMAKE_INSTALL_INCLUDEDIR "include" CACHE PATH "C header files (include)")
endif()

mark_as_advanced(CMAKE_INSTALL_BINDIR)
mark_as_advanced(CMAKE_INSTALL_LIBDIR)
mark_as_advanced(CMAKE_INSTALL_INCLUDEDIR)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

option(EVIO_BUILD_EXAMPLES "Build EVIO examples" OFF)
option(EVIO_BUILD_TESTS "Build EVIO tests" OFF)
option(EVIO_INSTALL_EXAMPLES "Install EVIO examples" OFF)


add_subdirectory("src/libsrc")
add_subdirectory("src/libsrc++")
add_subdirectory("src/execsrc")
add_subdirectory("doc/doxygen")

if (EVIO_BUILD_EXAMPLES)
  add_subdirectory("src/examples")
endif (EVIO_BUILD_EXAMPLES)

if (EVIO_BUILD_TESTS)
  add_subdirectory("src/test")
endif (EVIO_BUILD_TESTS)
