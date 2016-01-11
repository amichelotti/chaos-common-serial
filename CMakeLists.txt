cmake_minimum_required(VERSION 2.6)

IF (DEFINED ENV{CHAOS_BUNDLE} )
  include($ENV{CHAOS_BUNDLE}/tools/project_template/CMakeChaos.txt)
ELSE()
  MESSAGE(STATUS "STANDALONE, environment CHAOS_BUNDLE defined")
  IF (NOT CMAKE_INSTALL_PREFIX) 
    MESSAGE(fatal "you must add -DCMAKE_INSTALL_PREFIX=<dir>")
  ELSE()
    MESSAGE(STATUS "install prefix ${CMAKE_INSTALL_PREFIX}")
  ENDIF()
  INCLUDE_DIRECTORIES(. ../..)
  LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/build ${CMAKE_INSTALL_PREFIX}/lib)
ENDIF()

FILE(GLOB serial_src core/*.cpp)
FILE(GLOB serial_test_src test/*.cpp)
ADD_EXECUTABLE(test_serial test/test_serial.cpp)
ADD_EXECUTABLE(echo_serial test/echo_serial.cpp)

# -DPOSIX_SERIAL_COMM_DEBUG
# 

IF (DEBUG)

  ADD_DEFINITIONS(-DPOSIX_SERIAL_COMM_DEBUG -DOCEM_PROTOCOL_DEBUG )
ENDIF()

IF(FrameworkLib)
SET(libs chaos_common ${FrameworkLib})
ELSE()
SET(libs pthread)
ENDIF()


IF(BUILD_FORCE_STATIC)
  ADD_LIBRARY(common_serial STATIC ${serial_src})
ELSE()
  ADD_LIBRARY(common_serial SHARED ${serial_src})
ENDIF()
            
TARGET_LINK_LIBRARIES(common_serial common_debug ${libs})
TARGET_LINK_LIBRARIES(test_serial common_serial common_debug ${libs})

TARGET_LINK_LIBRARIES(echo_serial common_serial common_debug ${libs})

INSTALL_TARGETS(/bin test_serial)
INSTALL_TARGETS(/bin echo_serial)

FILE(GLOB ocem_src Ocem/*.h)    
INSTALL(FILES core/posixserialcomm_cwrap.h core/PosixSerialComm.h core/AbstractSerialComm.h DESTINATION include/common/serial/core) 
INSTALL(FILES serial.h DESTINATION include/common/serial/) 
INSTALL_TARGETS(/lib  common_serial)

INSTALL_TARGETS(/bin  test_serial)
INSTALL_TARGETS(/bin  echo_serial)

ADD_SUBDIRECTORY(models)

