#
# CMakeLists.txt
#
# Build the et package.
#
# Author: Maurik Holtrop
#
# Note: If you want to build the JAVA component, you need jni libraries. Uncomment the relevant lines to do this.
# 
cmake_minimum_required(VERSION 3.0)
project(et VERSION 16.1 LANGUAGES C CXX)
set(CMAKE_EXPORT_NO_PACKAGE_REGISTRY True)
set(CMAKE_CXX_STANDARD 11)

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
set(INSTALL_CMAKE_DIR lib/cmake/et)

# Make relative paths absolute (needed later on)
foreach(p LIB BIN INCLUDE CMAKE)
  set(var INSTALL_${p}_DIR)
  if(NOT IS_ABSOLUTE "${${var}}")
    set(${var} "${CMAKE_INSTALL_PREFIX}/${${var}}")
  endif()
endforeach()

set(INCLUDE_INSTALL_DIR ${INSTALL_INCLUDE_DIR}/et)
set(LIB_INSTALL_DIR ${INSTALL_LIB_DIR})

include_directories(src/libsrc )

message("------------------ ${PROJECT_NAME} ${PROJECT_VERSION} -----------------------")
message("We have CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")
message("CMAKE_CXX_FLAGS          = ${CMAKE_CXX_FLAGS}")
message("INCLUDE_INSTALL_DIR        = ${INCLUDE_INSTALL_DIR}")
message("LIB_INSTALL_DIR            = ${LIB_INSTALL_DIR}")
message("INSTALL_CMAKE_DIR          = ${INSTALL_CMAKE_DIR}")

# message("---------------------------------------------")

set(SOURCE_FILES
  src/libsrc/et_attachment.c
  src/libsrc/et_bridge.c
  src/libsrc/et_common.c
  src/libsrc/et_data.c
  src/libsrc/et_event.c
  src/libsrc/et_init.c
  src/libsrc/et_local.c
  src/libsrc/et_mem.c
  src/libsrc/et_network.c
  src/libsrc/et_noshare.c
  src/libsrc/et_openconfig.c
  src/libsrc/et_readwrite.c
  src/libsrc/et_remote.c
  src/libsrc/et_server.c
  src/libsrc/et_statconfig.c
  src/libsrc/et_station.c
  src/libsrc/et_sysconfig.c
  src/libsrc/et_system.c
  src/libsrc/etCommonNetwork.c
  )

set(HEADER_FILES_ET
  src/libsrc/et_data.h
  src/libsrc/et_network.h
  src/libsrc/et_private.h
  src/libsrc/et.h
  src/libsrc/etCommonNetwork.h
  )

set(SOURCE_FILES_REMOTE
  src/libsrc/et_remoteclient.c
  )

#    src/examples/et_consumer.c
#    src/examples/et_producer.c

#Generate the shared library from the sources
add_library(et SHARED ${SOURCE_FILES})

target_include_directories(et
        PUBLIC
        $<INSTALL_INTERFACE:include/et>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/libsrc>
        )

install(TARGETS et
        EXPORT  et-export
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

install(EXPORT et-export
        FILE etTargets.cmake
        DESTINATION ${INSTALL_CMAKE_DIR})

install(FILES ${HEADER_FILES_ET}
        DESTINATION ${INCLUDE_INSTALL_DIR})

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/etConfig.cmake
        DESTINATION ${INSTALL_CMAKE_DIR})
#
#
# The jni lib is not needed for this project, but can be
# build by uncommenting the following lines.
#
#set(SOURCE_FILES_JNI
#  src/libsrc/et_jni.c
#  src/libsrc/org_jlab_coda_et_EtJniAccess.h
#  )
#add_library(et-jni-SHARED ${SOURCE_FILES_JNI})
#add_dependencies(et-jni et)
#target_link_libraries(et-jni et)
#
# install(TARGETS et et-jni   DESTINATION lib)
#
add_executable(et_start src/execsrc/et_start.c)
add_dependencies(et_start et)
target_link_libraries(et_start et dl m pthread)

add_executable(et_monitor src/execsrc/et_monitor.c)
add_dependencies(et_monitor et)
target_link_libraries(et_monitor et dl m pthread)

add_executable(et_2_et src/execsrc/et_2_et.c)
add_dependencies(et_2_et et)
target_link_libraries(et_2_et et dl m pthread)

add_executable(et_killer src/execsrc/et_killer.c)
add_dependencies(et_killer et)
target_link_libraries(et_killer et dl m pthread)

add_executable(et_consumer src/examples/et_consumer.c)
add_dependencies(et_consumer et)
target_link_libraries(et_consumer et dl m pthread)


install(TARGETS et_start et_monitor et_2_et et_killer DESTINATION bin)

