cmake_minimum_required(VERSION 2.6)
project(common_serial_ocem)

IF (DEFINED ENV{CHAOS_BUNDLE} )
  include($ENV{CHAOS_BUNDLE}/tools/project_template/CMakeChaos.txt)
set (chaoslib chaos_uitoolkit chaos_common )
ELSE()
  MESSAGE(STATUS "STANDALONE, environment CHAOS_BUNDLE defined")
  IF (NOT CMAKE_INSTALL_PREFIX) 
    MESSAGE(fatal "you must add -DCMAKE_INSTALL_PREFIX=<dir>")
  ELSE()
    MESSAGE(STATUS "install prefix ${CMAKE_INSTALL_PREFIX}")
  ENDIF()
  INCLUDE_DIRECTORIES(. ../../../../)
  LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/build ${CMAKE_INSTALL_PREFIX}/lib)
ENDIF()


#     -DOCEM_CORE_DEBUG
ADD_DEFINITIONS(-DOCEM_PROTOCOL_BUFFER_DEBUG )

FILE(GLOB src *.cpp)    
IF(BUILD_FORCE_STATIC)
  ADD_LIBRARY(${PROJECT_NAME} STATIC ${src} )
ELSE()
  ADD_LIBRARY(${PROJECT_NAME} SHARED ${src})
ENDIF()

TARGET_LINK_LIBRARIES(${PROJECT_NAME} common_serial common_debug )
ADD_EXECUTABLE(ocem_protocol_test test/ocem_protocol_test.cpp)
TARGET_LINK_LIBRARIES(ocem_protocol_test ${PROJECT_NAME} common_serial common_debug ${chaoslib})


FILE(GLOB ocem_src *.h)    
 

INSTALL(FILES ${ocem_src} DESTINATION include/common/serial/models/Ocem ) 
INSTALL_TARGETS(/lib ${PROJECT_NAME})
INSTALL_TARGETS(/bin ocem_protocol_test)

