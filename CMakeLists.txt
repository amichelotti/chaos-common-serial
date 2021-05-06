cmake_minimum_required(VERSION 2.8)
project(common_serial)
find_package(chaos QUIET)
find_package(Boost MODULE QUIET COMPONENTS program_options system)
IF(Boost_FOUND)
	include_directories(${Boost_INCLUDE_DIRS})
	link_directories(${CMAKE_INSTALL_PREFIX}/lib)
ELSE()
	MESSAGE(STATUS "BOOST NOT FOUND skipping models compilation")
ENDIF()
IF(chaos_FOUND)
	
FILE(GLOB serial_src core/*.cpp)
ELSE()
MESSAGE(STATUS "CHAOS PACKAGES NOT FOUND ")
ADD_DEFINITIONS(-DNO_EXTERNAL_DEP)
FILE(GLOB serial_src core/PosixSerialComm.cpp core/AbstractSerialChannel.cpp core/posixserialcomm_cwrap.cpp)
ENDIF()


INCLUDE_DIRECTORIES( ../.. ${CMAKE_INSTALL_PREFIX}/include)


# -DPOSIX_SERIAL_COMM_DEBUG
# 
#-DPOSIX_SERIAL_COMM_DEBUG 

IF (DEBUG)

  ADD_DEFINITIONS(-DOCEM_PROTOCOL_DEBUG )
ENDIF()
IF(chaos_FOUND)
IF(FrameworkLib)
SET(libs ${FrameworkLib})
ELSE()
SET(libs pthread ${Boost_LIBRARIES})
ENDIF()
SET(libs libs common_debug)
ELSE()
SET(libs pthread)
ENDIF(chaos_FOUND)

IF(NOT CHAOS_STATIC)
ADD_LIBRARY(${PROJECT_NAME} SHARED ${serial_src})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${libs})

ELSE()
ADD_LIBRARY(${PROJECT_NAME} STATIC ${serial_src})
ENDIF()
IF(COMMON_TEST)
ADD_EXECUTABLE(test_serial test/test_serial.cpp)
ADD_EXECUTABLE(echo_serial test/echo_serial.cpp)
TARGET_LINK_LIBRARIES(test_serial ${PROJECT_NAME}  ${libs})
TARGET_LINK_LIBRARIES(echo_serial ${PROJECT_NAME} ${libs})
ENDIF()

TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${libs})

if(Boost_FOUND)
ADD_SUBDIRECTORY(models)
FILE(GLOB ocem_src Ocem/*.h)
ENDIF()
FILE(GLOB src_h core/*.h)
INSTALL(FILES ${src_h} DESTINATION include/common/serial/core) 
INSTALL(FILES serial.h DESTINATION include/common/serial/) 
#INSTALL_TARGETS(/lib  ${PROJECT_NAME})
INSTALL(TARGETS ${PROJECT_NAME}
        DESTINATION "lib"
        PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
IF(NOT BUILD_FORCE_STATIC)
  IF(COMMON_TEST)
    #INSTALL_TARGETS(/bin/serial  test_serial)
    INSTALL(TARGETS test_serial
        DESTINATION "bin/serial"
        PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    #INSTALL_TARGETS(/bin/serial  echo_serial)
    INSTALL(TARGETS echo_serial
        DESTINATION "bin/serial"
        PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
  ENDIF()
ENDIF()


