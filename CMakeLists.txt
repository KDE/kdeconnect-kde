project(kdeconnect)

cmake_minimum_required(VERSION 2.6)

find_package(KDE4 REQUIRED)

#if(CMAKE_COMPILER_IS_GNUCXX)
#    add_definitions(-std=gnu++0x)
#endif()

include(KDE4Defaults)
include_directories(${KDE4_INCLUDES})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(kded)
add_subdirectory(libkdeconnect)
add_subdirectory(kcm)
#add_subdirectory(kioslave)
add_subdirectory(plasmoid)

add_subdirectory(tests)
