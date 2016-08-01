cmake_minimum_required(VERSION 2.6)
project(common_serial)

FILE(GLOB serial_src core/*.cpp)
FILE(GLOB serial_test_src test/*.cpp)
ADD_EXECUTABLE(test_serial test/test_serial.cpp)
ADD_EXECUTABLE(echo_serial test/echo_serial.cpp)
INCLUDE_DIRECTORIES(. ../ )
# -DPOSIX_SERIAL_COMM_DEBUG
# 
#-DPOSIX_SERIAL_COMM_DEBUG 
IF (DEBUG)

  ADD_DEFINITIONS(-DOCEM_PROTOCOL_DEBUG )
ENDIF()

IF(FrameworkLib)
SET(libs chaos_common ${FrameworkLib})
ELSE()
SET(libs pthread)
ENDIF()


ADD_LIBRARY(${PROJECT_NAME} STATIC ${serial_src})

            
TARGET_LINK_LIBRARIES(${PROJECT_NAME} common_debug ${libs})
TARGET_LINK_LIBRARIES(test_serial ${PROJECT_NAME} common_debug ${libs})

TARGET_LINK_LIBRARIES(echo_serial ${PROJECT_NAME} common_debug ${libs})

INSTALL_TARGETS(/bin test_serial)
INSTALL_TARGETS(/bin echo_serial)

FILE(GLOB ocem_src Ocem/*.h)    
INSTALL(FILES core/posixserialcomm_cwrap.h core/PosixSerialComm.h core/AbstractSerialComm.h DESTINATION include/common/serial/core) 
INSTALL(FILES serial.h DESTINATION include/common/serial/) 
INSTALL_TARGETS(/lib  ${PROJECT_NAME})

INSTALL_TARGETS(/bin  test_serial)
INSTALL_TARGETS(/bin  echo_serial)

ADD_SUBDIRECTORY(models)

