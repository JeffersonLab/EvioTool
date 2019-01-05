cmake_minimum_required(VERSION 3.0)
project(et-16.1)

#include_directories($ENV{CODA}/common/include $ENV{JAVA_HOME}/include $ENV{JAVA_HOME}/include/linux $ENV{JAVA_HOME}/Headers)

IF(APPLE)
  include_directories(src/libsrc $ENV{JAVA_HOME}/include $ENV{JAVA_HOME}/include/darwin)
else()
  include_directories(src/libsrc $ENV{JAVA_HOME}/include  $ENV{JAVA_HOME}/include/linux)
endif()  

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

message("------------------ et-16.1 -----------------------")
message("We have CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")
# message("CMAKE_C_FLAGS_RELEASE    = ${CMAKE_C_FLAGS_RELEASE}")
# message("CMAKE_C_FLAGS_DEBUG      = ${CMAKE_C_FLAGS_DEBUG}")
# message("CMAKE_C_FLAGS            = ${CMAKE_C_FLAGS}")
message("CMAKE_CXX_FLAGS          = ${CMAKE_CXX_FLAGS}")
# message("CMAKE_CXX_FLAGS_RELEASE  = ${CMAKE_CXX_FLAGS_RELEASE}")
# message("CMAKE_CXX_FLAGS_DEBUG    = ${CMAKE_CXX_FLAGS_DEBUG}")
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

  src/libsrc/et_data.h
  src/libsrc/et_network.h
  src/libsrc/et_private.h
  src/libsrc/et.h
  src/libsrc/etCommonNetwork.h
  
  )

set(SOURCE_FILES_JNI
  src/libsrc/et_jni.c
  src/libsrc/org_jlab_coda_et_EtJniAccess.h
  )

set(SOURCE_FILES_REMOTE
  src/libsrc/et_remoteclient.c
)

#    src/examples/et_consumer.c
#    src/examples/et_producer.c

#Generate the shared library from the sources
add_library(et-16.1     SHARED ${SOURCE_FILES})
add_library(et-jni-16.1 SHARED ${SOURCE_FILES_JNI})

add_dependencies(et-jni-16.1 et-16.1)
target_link_libraries(et-jni-16.1 et-16.1)

add_executable(et_start src/execsrc/et_start.c)
add_dependencies(et_start et-16.1)
target_link_libraries(et_start et-16.1)

add_executable(et_monitor src/execsrc/et_monitor.c)
add_dependencies(et_monitor et-16.1)
target_link_libraries(et_monitor et-16.1)

add_executable(et_2_et src/execsrc/et_2_et.c)
add_dependencies(et_2_et et-16.1)
target_link_libraries(et_2_et et-16.1)

add_executable(et_killer src/execsrc/et_killer.c)
add_dependencies(et_killer et-16.1)
target_link_libraries(et_killer et-16.1)

install(TARGETS et-16.1 et-jni-16.1   DESTINATION lib)
install(TARGETS et_start et_monitor et_2_et et_killer DESTINATION bin)

