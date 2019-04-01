cmake_minimum_required(VERSION 2.8)
project(common_serial)
find_package(chaos)
find_package(Boost MODULE REQUIRED REQUIRED COMPONENTS program_options system)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${CMAKE_INSTALL_PREFIX}/lib)
FILE(GLOB serial_src core/*.cpp)

INCLUDE_DIRECTORIES( ../.. ${CMAKE_INSTALL_PREFIX}/include)


# -DPOSIX_SERIAL_COMM_DEBUG
# 
#-DPOSIX_SERIAL_COMM_DEBUG 

IF (DEBUG)

  ADD_DEFINITIONS(-DOCEM_PROTOCOL_DEBUG )
ENDIF()

IF(FrameworkLib)
SET(libs ${FrameworkLib})
ELSE()
SET(libs pthread ${Boost_LIBRARIES})
ENDIF()


ADD_LIBRARY(${PROJECT_NAME} STATIC ${serial_src})
ADD_EXECUTABLE(test_serial test/test_serial.cpp)
ADD_EXECUTABLE(echo_serial test/echo_serial.cpp)

TARGET_LINK_LIBRARIES(${PROJECT_NAME} common_debug ${libs})
TARGET_LINK_LIBRARIES(test_serial ${PROJECT_NAME} common_debug ${libs})
TARGET_LINK_LIBRARIES(echo_serial ${PROJECT_NAME} common_debug ${libs})

ADD_SUBDIRECTORY(models)



FILE(GLOB ocem_src Ocem/*.h)
FILE(GLOB src_h core/*.h)
INSTALL(FILES ${src_h} DESTINATION include/common/serial/core) 
INSTALL(FILES serial.h DESTINATION include/common/serial/) 
INSTALL_TARGETS(/lib  ${PROJECT_NAME})
IF(NOT BUILD_FORCE_STATIC)
  INSTALL_TARGETS(/bin/serial  test_serial)
  INSTALL_TARGETS(/bin/serial  echo_serial)
ENDIF()


