cmake_minimum_required(VERSION 2.6)
project(common_serial_ocem)
#     -DOCEM_CORE_DEBUG -DOCEM_PROTOCOL_DEBUG
ADD_DEFINITIONS( -DOCEM_PROTOCOL_BUFFER_DEBUG )

FILE(GLOB src *.cpp)    

ADD_LIBRARY(${PROJECT_NAME} STATIC ${src} )
TARGET_LINK_LIBRARIES(${PROJECT_NAME} common_serial common_debug )
ADD_EXECUTABLE(ocem_protocol_test test/ocem_protocol_test.cpp)
TARGET_LINK_LIBRARIES(ocem_protocol_test ${PROJECT_NAME} common_misc_driver common_debug ${chaoslib})


FILE(GLOB ocem_src *.h)    
 

INSTALL(FILES ${ocem_src} DESTINATION include/common/serial/models/Ocem ) 
INSTALL_TARGETS(/lib ${PROJECT_NAME})
INSTALL_TARGETS(/bin ocem_protocol_test)

