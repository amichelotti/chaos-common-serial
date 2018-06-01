cmake_minimum_required(VERSION 2.6)
project(common_serial)
find_package(Boost MODULE REQUIRED REQUIRED COMPONENTS program_options system)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${CMAKE_INSTALL_PREFIX}/lib)
FILE(GLOB serial_src core/*.cpp)
FILE(GLOB serial_test_src test/*.cpp)
ADD_EXECUTABLE(test_serial test/test_serial.cpp)
ADD_EXECUTABLE(echo_serial test/echo_serial.cpp)
INCLUDE_DIRECTORIES( ../.. ${CMAKE_INSTALL_PREFIX}/include)


# -DPOSIX_SERIAL_COMM_DEBUG
# 
#-DPOSIX_SERIAL_COMM_DEBUG 
ADD_DEFINITIONS(-O3 -fPIC)
IF (DEBUG)

  ADD_DEFINITIONS(-DOCEM_PROTOCOL_DEBUG )
ENDIF()

IF(FrameworkLib)
SET(libs chaos_common ${FrameworkLib})
ELSE()
SET(libs pthread ${Boost_LIBRARIES})
ENDIF()


ADD_LIBRARY(${PROJECT_NAME} STATIC ${serial_src})


MESSAGE(STATUS ${Boost_LIBRARIES})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} common_debug ${libs})
TARGET_LINK_LIBRARIES(test_serial ${PROJECT_NAME} common_debug ${libs})

TARGET_LINK_LIBRARIES(echo_serial ${PROJECT_NAME} common_debug ${libs})


FILE(GLOB ocem_src Ocem/*.h)
FILE(GLOB src_h core/*.h)
INSTALL(FILES ${src_h} DESTINATION include/common/serial/core) 
INSTALL(FILES serial.h DESTINATION include/common/serial/) 
INSTALL_TARGETS(/lib  ${PROJECT_NAME})

INSTALL_TARGETS(/bin/serial  test_serial)
INSTALL_TARGETS(/bin/serial  echo_serial)

ADD_SUBDIRECTORY(models)

