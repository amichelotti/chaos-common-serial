cmake_minimum_required(VERSION 2.6)
include($ENV{CHAOS_BUNDLE}/tools/project_template/CMakeChaos.txt)
FILE(GLOB serial_src core/*.cpp Ocem/*.cpp)
FILE(GLOB serial_test_src test/*.cpp)
ADD_EXECUTABLE(test_serial test/test_serial.cpp)
ADD_EXECUTABLE(echo_serial test/echo_serial.cpp)
IF(BUILD_FORCE_STATIC)
  ADD_LIBRARY(common_serial STATIC ${serial_src})
ELSE()
  ADD_LIBRARY(common_serial SHARED ${serial_src})
ENDIF()
            
TARGET_LINK_LIBRARIES(common_serial common_debug)
TARGET_LINK_LIBRARIES(test_serial common_serial pthread common_debug boost_program_options boost_regex)

TARGET_LINK_LIBRARIES(echo_serial common_serial pthread common_debug boost_program_options boost_regex)

INSTALL_TARGETS(/bin test_serial)
INSTALL_TARGETS(/bin echo_serial)

FILE(GLOB ocem_src Ocem/*.h)    
INSTALL(FILES core/posixserialcomm_cwrap.h core/PosixSerialComm.h core/AbstractSerialComm.h DESTINATION include/common/serial/core) 
INSTALL(FILES serial.h DESTINATION include/common/serial/) 
INSTALL(FILES ${ocem_src} DESTINATION include/common/serial/Ocem ) 
INSTALL_TARGETS(/lib  common_serial)

INSTALL_TARGETS(/bin  test_serial)
INSTALL_TARGETS(/bin  echo_serial)

