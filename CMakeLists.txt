#
# CMakeLists.txt
#
# This file allows you to build the EvioTool, evio and et packages in a single step with cmake.
# Building this package requires the ROOT system from CERN.
#
# Author: Maurik Holtrop
#
cmake_minimum_required(VERSION 3.5)
project(EvioTool VERSION 1.0 LANGUAGES CXX)
set(CMAKE_EXPORT_NO_PACKAGE_REGISTRY True)

set(CMAKE_CXX_STANDARD 17)

#---Locate the ROOT package and defines a number of variables (e.g. ROOT_INCLUDE_DIRS)
find_package(ROOT REQUIRED COMPONENTS Core)
message(STATUS "ROOT found at: ${ROOT_DIR}")

#
# Set a default build type if none was specified
#
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
set(DEF_INSTALL_CMAKE_DIR lib/cmake/EvioTool)
set(INSTALL_CMAKE_DIR ${DEF_INSTALL_CMAKE_DIR} CACHE PATH "Installation directory for CMake files")

# Make relative paths absolute (needed later on)
foreach(p LIB BIN INCLUDE CMAKE)
  set(var INSTALL_${p}_DIR)
  if(NOT IS_ABSOLUTE "${${var}}")
    set(${var} "${CMAKE_INSTALL_PREFIX}/${${var}}")
  endif()
endforeach()


set(INCLUDE_INSTALL_DIR ${INSTALL_INCLUDE_DIR}/EvioTool)
set(LIB_INSTALL_DIR ${INSTALL_LIB_DIR})
set(SYSCONFIG_INSTALL_DIR ${INSTALL_CMAKE_DIR})

#
# Shortcut to control which version of EVIO or ET is linked against.
#
#set(EVIO_LIBRARY evio-4.4.6)
set(EVIO_LIBRARY evio-5.1)
add_subdirectory(${EVIO_LIBRARY})

set(ET_LIBRARY   et-14.0-clon)
set(ET_INCLUDE_DIRECTORY ${ET_LIBRARY}/et.s)  # Note: location of the et code is not consistent.
add_subdirectory(${ET_LIBRARY})

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG")
set(CMAKE_C_FLAGS_DEBUG "-g -DDEBUG")

message("------------- EvioTool ---------------------------")
message("We have CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")
message("CMAKE_C_FLAGS_RELEASE    = ${CMAKE_C_FLAGS_RELEASE}")
message("CMAKE_C_FLAGS_DEBUG      = ${CMAKE_C_FLAGS_DEBUG}")
message("CMAKE_C_FLAGS            = ${CMAKE_C_FLAGS}")
message("CMAKE_CXX_FLAGS          = ${CMAKE_CXX_FLAGS}")
message("CMAKE_CXX_FLAGS_RELEASE  = ${CMAKE_CXX_FLAGS_RELEASE}")
message("CMAKE_CXX_FLAGS_DEBUG    = ${CMAKE_CXX_FLAGS_DEBUG}")
message("INCLUDE_INSTALL_DIR        = ${INCLUDE_INSTALL_DIR}")
message("LIB_INSTALL_DIR            = ${LIB_INSTALL_DIR}")
message("INSTALL_CMAKE_DIR          = ${INSTALL_CMAKE_DIR}")
message("--------------------------------------------------")

#---Define useful ROOT functions and macros (e.g. ROOT_GENERATE_DICTIONARY)
include(${ROOT_USE_FILE})

include_directories(${EVIO_LIBRARY}/src/libsrc ${ET_INCLUDE_DIRECTORY} EvioTool HPSEvioReader)

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
  EvioTool/Leaf.cc
  EvioTool/FADCdata.cc
  )

set(HEADER_FILES_EVIOTOOL
        EvioTool/EvioTool.h
        EvioTool/FADCdata.h
        EvioTool/Bank.h
        EvioTool/Leaf.h
        )

set(DICTS_EVIOTOOL
  EvioTool_Dict.cxx
  Leaf_Dict.cxx
  FADCdata_Dict.cxx
  )

set(SOURCE_FILES_HPSEVIOREADER
  HPSEvioReader/Cluster.cpp
  HPSEvioReader/EcalCluster.cpp
  HPSEvioReader/EcalGTPCluster.cpp
  HPSEvioReader/EcalHit.cpp
  HPSEvioReader/EcalBank.cpp
  HPSEvioReader/HPSEvioReader.cpp
  HPSEvioReader/Header.cpp
  HPSEvioReader/Headbank.cpp
  HPSEvioReader/SVTBank.cc
  HPSEvioReader/SVTBank2016.cc
  HPSEvioReader/TSBank.cpp
  HPSEvioReader/TriggerConfig.cpp
  HPSEvioReader/VTPBank.cpp
  )

set(HEADER_FILES_HPSEVIOREADER
        HPSEvioReader/Cluster.h
        HPSEvioReader/EcalCluster.h
        HPSEvioReader/EcalGTPCluster.h
        HPSEvioReader/EcalHit.h
        HPSEvioReader/EcalBank.h
        HPSEvioReader/HPSEvioReader.h
        HPSEvioReader/Header.h
        HPSEvioReader/Headbank.h
        HPSEvioReader/SVTBank.h
        HPSEvioReader/SVTBank2016.h
        HPSEvioReader/TSBank.h
        HPSEvioReader/TriggerConfig.h
        HPSEvioReader/VTPBank.h
        )

ROOT_GENERATE_DICTIONARY(EvioTool_Dict  EvioTool.h LINKDEF EvioTool/EvioTool_LinkDef.h MODULE EvioTool)
ROOT_GENERATE_DICTIONARY(HPSEvioReader_Dict HPSEvioReader.h  LINKDEF HPSEvioReader/HPSEvioReader_LinkDef.h MODULE HPSEvioReader)

#### Build package configuration files, see: https://cmake.org/cmake/help/v3.18/manual/cmake-packages.7.html
##### https://cmake.org/cmake/help/v3.4/module/CMakePackageConfigHelpers.html
##### Example/Blog:
##### https://levelup.gitconnected.com/simple-modern-cmake-tutorial-b0c1c362cd2c
include(CMakePackageConfigHelpers)

############################################################
#        EvioTool library
#########################################################3##
#---Create a shared library with geneated dictionary
#add_library(EvioTool SHARED ${SOURCE_FILES_EVIOTOOL} ${DICTS_EVIOTOOL} )
add_library(EvioTool SHARED ${SOURCE_FILES_EVIOTOOL} EvioTool_Dict.cxx )

# Prepare the include directory for export.
target_include_directories(EvioTool
        PUBLIC
        $<INSTALL_INTERFACE:include/EvioTool>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/EvioTool>
        )

target_link_libraries(EvioTool PUBLIC ${ROOT_LIBRARIES} evio et)

install(TARGETS EvioTool
        EXPORT EvioTool-export
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

install(EXPORT EvioTool-export
        FILE EvioToolTargets.cmake
        DESTINATION ${INSTALL_CMAKE_DIR})

install(FILES ${PROJECT_BINARY_DIR}/libEvioTool_rdict.pcm DESTINATION lib)

install(FILES ${HEADER_FILES_EVIOTOOL}
        DESTINATION ${INCLUDE_INSTALL_DIR})

export(TARGETS EvioTool evio et
        FILE "${PROJECT_BINARY_DIR}/EvioToolTargets.cmake")
#
#
export(PACKAGE EvioTool)

configure_package_config_file( cmake/EvioToolConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/EvioToolConfig.cmake
        INSTALL_DESTINATION ${INSTALL_CMAKE_DIR}
        PATH_VARS INCLUDE_INSTALL_DIR SYSCONFIG_INSTALL_DIR)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/EvioToolConfig.cmake
        DESTINATION ${INSTALL_CMAKE_DIR})

############################################################
#        EvioTool Test Executables
############################################################
add_executable(EvioTool_Test EvioTool_Test/EvioTool_Test.cpp)
add_dependencies(EvioTool_Test EvioTool)
target_link_libraries(EvioTool_Test EvioTool)

############################################################
#        HPSEvioReader library
############################################################

add_library(HPSEvioReader SHARED ${SOURCE_FILES_HPSEVIOREADER} HPSEvioReader_Dict.cxx)
target_include_directories(HPSEvioReader
                           PUBLIC
                           $<INSTALL_INTERFACE:include/EvioTool>
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/HPSEvioReader>
                           )

target_link_libraries(HPSEvioReader EvioTool ${ROOT_LIBRARIES} evio)

#
# Note: The HPSEvioReader piggy-backs onto the EvioTool-export. That way
# you can simply import EvioTool and also get the HPSEvioReader.
# We may need to separate these two completely at some point.
#
install(TARGETS HPSEvioReader
        EXPORT  EvioTool-export
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

install(FILES ${PROJECT_BINARY_DIR}/libHPSEvioReader_rdict.pcm  DESTINATION lib)

install(FILES ${HEADER_FILES_HPSEVIOREADER}
        DESTINATION ${INCLUDE_INSTALL_DIR})

############################################################
#        HPSEvioReader Executables
#########################################################3##

add_executable(HPSEvioReader_Test HPSEvioReader_Test/HPSEvioReader_Test.cpp)
add_dependencies(HPSEvioReader_Test HPSEvioReader EvioTool)
target_link_libraries(HPSEvioReader_Test HPSEvioReader EvioTool)

add_executable(HPS_Trigger_Test HPS_Trigger_Test/HPS_Trigger_Test.cpp HPS_Trigger_Test/TriggerHistograms.cpp  HPS_Trigger_Test/TriggerChecker.cpp )
add_dependencies(HPS_Trigger_Test HPSEvioReader EvioTool)
target_link_libraries(HPS_Trigger_Test HPSEvioReader EvioTool)

add_executable(HPS_Trigger_Filter HPS_Trigger_Filter/HPS_Trigger_Filter.cpp )
add_dependencies(HPS_Trigger_Filter HPSEvioReader EvioTool)
target_link_libraries(HPS_Trigger_Filter HPSEvioReader EvioTool)

install(TARGETS EvioTool_Test         DESTINATION bin)
install(TARGETS HPSEvioReader_Test  DESTINATION bin)
install(TARGETS HPS_Trigger_Test  DESTINATION bin)
install(TARGETS HPS_Trigger_Filter  DESTINATION bin)

############################################################
#        ET Testing Code.
#########################################################3##

add_executable(et_evio_blaster et_evio_blaster/et_evio_blaster.cc )
target_link_libraries(et_evio_blaster evio et)
install(TARGETS et_evio_blaster     DESTINATION bin)

add_executable(et_evio_test_consumer et_evio_test_consumer/et_evio_test_consumer.cc )
target_link_libraries(et_evio_test_consumer evio et)
install(TARGETS et_evio_test_consumer     DESTINATION bin)