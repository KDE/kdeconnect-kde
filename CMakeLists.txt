project(kdeconnect)

cmake_minimum_required(VERSION 2.6)

find_package(KDE4 REQUIRED)
find_package(QJSON REQUIRED)
find_package(QCA2 REQUIRED)

#if(CMAKE_COMPILER_IS_GNUCXX)
#    add_definitions(-std=gnu++0x)
#endif()

include(GenerateExportHeader)
include(KDE4Defaults)
include_directories(${KDE4_INCLUDES})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(core)
add_subdirectory(kcm)
add_subdirectory(kio)
add_subdirectory(icon)
add_subdirectory(interfaces)

add_subdirectory(kded)
add_subdirectory(plasmoid)
add_subdirectory(cli)

add_subdirectory(tests)
