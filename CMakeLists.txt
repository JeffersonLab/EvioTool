#
# CMakeLists.txt
#
# This file allows you to build the EvioTool, evio and et packages in a single step with cmake.
# Building this package requires the ROOT system from CERN.
#
# Author: Maurik Holtrop
#


cmake_minimum_required(VERSION 3.0)
project(EvioTool)

#---Locate the ROOT package and defines a number of variables (e.g. ROOT_INCLUDE_DIRS)
find_package(ROOT REQUIRED COMPONENTS Core)
message(STATUS "ROOT found at: ${ROOT_DIR}")

#
# Set a default build type if none was specified
set(default_build_type "RELEASE")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

#
# Shortcut to control which version of EVIO or ET is linked against.
#
#set(EVIO_LIBRARY evio-4.4.6)
set(EVIO_LIBRARY evio-5.1)
add_subdirectory(${EVIO_LIBRARY})

set(ET_LIBRARY   et-16.1)
add_subdirectory(${ET_LIBRARY})


set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

message("---------------------------------------------")
message("We have CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")
message("CMAKE_C_FLAGS_RELEASE    = ${CMAKE_C_FLAGS_RELEASE}")
message("CMAKE_C_FLAGS_DEBUG      = ${CMAKE_C_FLAGS_DEBUG}")
message("CMAKE_C_FLAGS            = ${CMAKE_C_FLAGS}")
message("CMAKE_CXX_FLAGS          = ${CMAKE_CXX_FLAGS}")
message("CMAKE_CXX_FLAGS_RELEASE  = ${CMAKE_CXX_FLAGS_RELEASE}")
message("CMAKE_CXX_FLAGS_DEBUG    = ${CMAKE_CXX_FLAGS_DEBUG}")
message("---------------------------------------------")

#---Define useful ROOT functions and macros (e.g. ROOT_GENERATE_DICTIONARY)
include(${ROOT_USE_FILE})

include_directories(evio-5.1/src/libsrc EvioTool HPSEvioReader) 

if(APPLE)
  # use, i.e. don't skip the full RPATH for the build tree
  SET(CMAKE_SKIP_BUILD_RPATH  FALSE)
  
  # when building, don't use the install RPATH already
  # (but later on when installing)
  SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
  
  SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
  
  # add the automatically determined parts of the RPATH
  # which point to directories outside the build tree to the install RPATH
  SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
  
  
  # the RPATH to be used when installing, but only if it's not a system directory
  LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
  IF("${isSystemDir}" STREQUAL "-1")
    SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
  ENDIF("${isSystemDir}" STREQUAL "-1")
  
endif()

set(SOURCE_FILES_EVIOTOOL
  EvioTool/Bank.cc
  EvioTool/EvioTool.cc
  EvioTool/FADCdata.h
  EvioTool/Leaf.cc
  EvioTool/Bank.h
  EvioTool/EvioTool.h
  EvioTool/FADCdata.cc
  EvioTool/Leaf.h  
  )

set(DICTS_EVIOTOOL
  EvioTool_Dict.cxx
  Leaf_Dict.cxx
  FADCdata_Dict.cxx
  )

set(SOURCE_FILES_HPSEVIOREADER
  HPSEvioReader/Cluster.cpp
  HPSEvioReader/Cluster.h
  HPSEvioReader/EcalCluster.cpp
  HPSEvioReader/EcalCluster.h
  HPSEvioReader/EcalGTPCluster.cpp
  HPSEvioReader/EcalGTPCluster.h
  HPSEvioReader/EcalHit.cpp
  HPSEvioReader/EcalHit.h
  HPSEvioReader/EcalData.cpp
  HPSEvioReader/EcalData.h
  HPSEvioReader/HPSEvioReader.cpp
  HPSEvioReader/HPSEvioReader.h
  HPSEvioReader/Header.cpp
  HPSEvioReader/Header.h
  HPSEvioReader/Headbank.cpp
  HPSEvioReader/Headbank.h
  HPSEvioReader/SVTbank.cc
  HPSEvioReader/SVTbank.h
  HPSEvioReader/TIData.cpp
  HPSEvioReader/TIData.h
  HPSEvioReader/TriggerConfig.cpp
  HPSEvioReader/TriggerConfig.h
  HPSEvioReader/VTPData.cpp
  HPSEvioReader/VTPData.h
  )

#set(DICTS_HPSEVIOREADER
#  SVTbank_Dict.cxx
#  )


ROOT_GENERATE_DICTIONARY(EvioTool_Dict      EvioTool/EvioTool.h LINKDEF EvioTool/EvioTool_LinkDef.h MODULE EvioTool)
ROOT_GENERATE_DICTIONARY(HPSEvioReader_Dict HPSEvioReader/HPSEvioReader.h LINKDEF HPSEvioReader/HPSEvioReader_LinkDef.h MODULE HPSEvioReader)

#---Create a shared library with geneated dictionary
#add_library(EvioTool SHARED ${SOURCE_FILES_EVIOTOOL} ${DICTS_EVIOTOOL} )
add_library(EvioTool SHARED ${SOURCE_FILES_EVIOTOOL} EvioTool_Dict.cxx )
target_link_libraries(EvioTool ${ROOT_LIBRARIES} ${EVIO_LIBRARY})

add_executable(EvioTool_Test EvioTool_Test/EvioTool_Test.cpp)
add_dependencies(EvioTool_Test EvioTool)
target_link_libraries(EvioTool_Test EvioTool ${EVIO_LIBRARY})

add_library(HPSEvioReader SHARED ${SOURCE_FILES_HPSEVIOREADER} HPSEvioReader_Dict.cxx)
target_link_libraries(HPSEvioReader EvioTool ${ROOT_LIBRARIES} ${EVIO_LIBRARY})

add_executable(HPSEvioReader_Test HPSEvioReader_Test/HPSEvioReader_Test.cpp)
add_dependencies(HPSEvioReader_Test HPSEvioReader EvioTool)
target_link_libraries(HPSEvioReader_Test HPSEvioReader EvioTool ${EVIO_LIBRARY})



install(TARGETS EvioTool              DESTINATION lib)
install(FILES ${PROJECT_BINARY_DIR}/libEvioTool_rdict.pcm DESTINATION lib)
install(TARGETS EvioTool_Test         DESTINATION bin)

install(TARGETS HPSEvioReader       DESTINATION lib)
install(FILES ${PROJECT_BINARY_DIR}/libHPSEvioReader_rdict.pcm  DESTINATION lib)
install(TARGETS HPSEvioReader_Test  DESTINATION bin)