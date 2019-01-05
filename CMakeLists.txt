cmake_minimum_required(VERSION 3.0)
project(EvioTool)

set(CMAKE_BUILD_TYPE Release)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

message("---------------------------------------------")
message("We have CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")
message("CMAKE_C_FLAGS_RELEASE    = ${CMAKE_C_FLAGS_RELEASE}")
message("CMAKE_C_FLAGS_DEBUG      = ${CMAKE_C_FLAGS_DEBUG}")
message("CMAKE_C_FLAGS            = ${CMAKE_C_FLAGS}")
message("CMAKE_CXX_FLAGS          = ${CMAKE_CXX_FLAGS}")
message("CMAKE_CXX_FLAGS_RELEASE  = ${CMAKE_CXX_FLAGS_RELEASE}")
message("CMAKE_CXX_FLAGS_DEBUG    = ${CMAKE_CXX_FLAGS_DEBUG}")
message("---------------------------------------------")

#---Locate the ROOT package and defines a number of variables (e.g. ROOT_INCLUDE_DIRS)
find_package(ROOT REQUIRED COMPONENTS Core)
message(STATUS "ROOT found at: ${ROOT_DIR}")

#---Define useful ROOT functions and macros (e.g. ROOT_GENERATE_DICTIONARY)
include(${ROOT_USE_FILE})

add_subdirectory(evio-5.1)
add_subdirectory(et-16.1)

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
  EvioTool/EvioParser.cc
  EvioTool/FADCdata.h
  EvioTool/Leaf.cc
  EvioTool/Bank.h
  EvioTool/EvioParser.h
  EvioTool/FADCdata.cc
  EvioTool/Leaf.h  
  )

set(DICTS_EVIOTOOL
  EvioParser_Dict.cxx
  Leaf_Dict.cxx
  FADCdata_Dict.cxx
  )

set(SOURCE_FILES_HPSEVIOREADER
  HPSEvioReader/SVTbank.cc
  HPSEvioReader/SVTbank.h
  )

set(DICTS_HPSEVIOREADER
  SVTbank_Dict.cxx
  )

ROOT_GENERATE_DICTIONARY(EvioParser_Dict EvioTool/EvioParser.h EvioTool/Bank.h  LINKDEF EvioTool/EvioParser_LinkDef.h)
ROOT_GENERATE_DICTIONARY(Leaf_Dict EvioTool/Leaf.h LINKDEF EvioTool/Leaf_LinkDef.h)
ROOT_GENERATE_DICTIONARY(FADCdata_Dict EvioTool/FADCdata.h LINKDEF EvioTool/FADCdata_LinkDef.h)

ROOT_GENERATE_DICTIONARY(SVTbank_Dict HPSEvioReader/SVTbank.h LINKDEF HPSEvioReader/SVTbank_LinkDef.h)


#---Create a shared library with geneated dictionary
add_library(EvioTool SHARED ${SOURCE_FILES_EVIOTOOL} ${DICTS_EVIOTOOL} )
target_link_libraries(EvioTool ${ROOT_LIBRARIES} evio-5.1)

add_executable(EvioTool_Test EvioTool_Test/EvioTool_Test.cpp)
add_dependencies(EvioTool_Test EvioTool)
target_link_libraries(EvioTool_Test EvioTool evio-5.1)

add_library(HPSEvioReader SHARED ${SOURCE_FILES_HPSEVIOREADER} ${DICTS_HPSEVIOREADER})
target_link_libraries(HPSEvioReader EvioTool ${ROOT_LIBRARIES} evio-5.1)

add_executable(HPSEvioReader_Test HPSEvioReader_Test/HPSEvioReader_Test.cpp)
add_dependencies(HPSEvioReader_Test HPSEvioReader EvioTool)
target_link_libraries(HPSEvioReader_Test HPSEvioReader EvioTool evio-5.1)

install(TARGETS EvioTool            DESTINATION lib)
install(TARGETS EvioTool_Test       DESTINATION bin)

install(TARGETS HPSEvioReader       DESTINATION lib)
install(TARGETS HPSEvioReader_Test  DESTINATION bin)