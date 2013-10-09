cmake_minimum_required(VERSION 2.6)
option(BUILD_FORCE_32 "Set to ON to enable 32 bit compilation" OFF)


IF( ($ENV{CHAOS32}) OR (BUILD_FORCE_32) )
    MESSAGE(STATUS "Enabling 32 bit Compilation")
    set (CMAKE_C_FLAGS "-m32")
    set (CMAKE_CXX_FLAGS "-m32")
    set (CMAKE_LINK_FLAGS "-m32")
ENDIF()

if( DEFINED ENV{CHAOS_FRAMEWORK} ) 
    MESSAGE(STATUS "Using $ENV{CHAOS_FRAMEWORK}")
      set (PROJECT_SOURCE_DIR $ENV{CHAOS_FRAMEWORK})
ELSE()
    MESSAGE(FATAL_ERROR "Please set CHAOS_FRAMEWORK environment")
ENDIF()

set (CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR}/usr/local)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)
     
INCLUDE_DIRECTORIES(. source test ${PROJECT_SOURCE_DIR}/usr/local/include)
LINK_DIRECTORIES(${PROJECT_SOURCE_DIR}/usr/local/lib)

ADD_DEFINITIONS(-g -O2 -Wall -DDEBUG)


#### put your libcommon_serial LIBRARY source
### files in source
FILE(GLOB serial_src source/*.cpp)
###

ADD_LIBRARY(common_serial SHARED ${serial_src})
TARGET_LINK_LIBRARIES(common_serial pthread common_debug)

## gets all sources in test, a main function must be present
FILE(GLOB serial_test_src test/*.cpp)
###
ADD_EXECUTABLE(test_serial test/test_serial.cpp)
ADD_EXECUTABLE(echo_serial test/echo_serial.cpp)

TARGET_LINK_LIBRARIES(test_serial common_serial pthread common_debug boost_program_options boost_regex)
TARGET_LINK_LIBRARIES(echo_serial common_serial pthread common_debug boost_program_options boost_regex)
INSTALL(FILES source/posixserialcomm_cwrap.h source/PosixSerialComm.h DESTINATION include/common ) 
INSTALL_TARGETS(/lib  common_serial)
INSTALL_TARGETS(/bin test_serial)
INSTALL_TARGETS(/bin echo_serial)
