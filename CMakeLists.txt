cmake_minimum_required(VERSION 2.6)
option(BUILD_FORCE_32 "Set to ON to enable 32 bit compilation" OFF)


IF( (DEFINED ENV{CHAOS32}) OR (BUILD_FORCE_32) )
    MESSAGE(STATUS "Enabling 32 bit Compilation")
    set (CMAKE_C_FLAGS "-m32")
    set (CMAKE_CXX_FLAGS "-m32")
    set (CMAKE_LINK_FLAGS "-m32")
ENDIF()


FILE(GLOB serial_src core/*.cpp Ocem/*.cpp)
###





if( DEFINED ENV{CHAOS_FRAMEWORK} ) 
    MESSAGE(STATUS "Using $ENV{CHAOS_FRAMEWORK}")
      set (PROJECT_SOURCE_DIR $ENV{CHAOS_FRAMEWORK})
      INCLUDE_DIRECTORIES(../../ test ${PROJECT_SOURCE_DIR}/usr/local/include)
      LINK_DIRECTORIES(${PROJECT_SOURCE_DIR}/usr/local/lib)
# -DPOSIX_SERIAL_COMM_DEBUG  -DOCEM_PROTOCOL_DEBUG
      ADD_DEFINITIONS(-g   -O2 -Wall)
      
      FILE(GLOB serial_test_src test/*.cpp)
      ADD_EXECUTABLE(test_serial test/test_serial.cpp)
      ADD_EXECUTABLE(echo_serial test/echo_serial.cpp)
      ADD_LIBRARY(common_serial SHARED ${serial_src})
      TARGET_LINK_LIBRARIES(common_serial common_debug)
      TARGET_LINK_LIBRARIES(test_serial common_serial pthread common_debug boost_program_options boost_regex)

      TARGET_LINK_LIBRARIES(echo_serial common_serial pthread common_debug boost_program_options boost_regex)

      INSTALL_TARGETS(/bin test_serial)
      INSTALL_TARGETS(/bin echo_serial)
ELSE()
     MESSAGE(STATUS "NO CHAOS_FRAMEWORK environment defined, disabled test and debug targets")
     LINK_DIRECTORIES(build)
     ADD_DEFINITIONS(-g -O2 -Wall)
     set (PROJECT_SOURCE_DIR .)
     INCLUDE_DIRECTORIES(../../ source test ${PROJECT_SOURCE_DIR}/usr/local/include)
     ADD_LIBRARY(common_serial SHARED ${serial_src})
     TARGET_LINK_LIBRARIES(common_serial pthread)
     
ENDIF()


set (CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR}/usr/local)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)

FILE(GLOB ocem_src Ocem/*.h)    
INSTALL(FILES core/posixserialcomm_cwrap.h core/PosixSerialComm.h core/AbstractSerialComm.h DESTINATION include/common/serial/core) 
INSTALL(FILES serial.h DESTINATION include/common/serial/) 
INSTALL(FILES ${ocem_src} DESTINATION include/common/serial/Ocem ) 
INSTALL_TARGETS(/lib  common_serial)
INSTALL_TARGETS(/bin  test_serial)
INSTALL_TARGETS(/bin  echo_serial)

