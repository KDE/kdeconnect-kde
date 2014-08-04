project(kdeconnect)

cmake_minimum_required(VERSION 2.8.12)

find_package(ECM 0.0.9 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} ${CMAKE_SOURCE_DIR}/cmake)

find_package(Qt5 5.2 REQUIRED COMPONENTS Quick Test)
find_package(KF5 REQUIRED COMPONENTS I18n KIO Notifications ConfigWidgets DBusAddons KCMUtils KDELibs4Support)
find_package(QCA2 REQUIRED)
find_package(QJSON REQUIRED)

include(KDEInstallDirs)
include(KDECompilerSettings)
include(KDECMakeSettings)
include(ECMAddTests)
include(ECMSetupVersion)
include(ECMInstallIcons)
include(FeatureSummary)

include(GenerateExportHeader)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(core)
add_subdirectory(kcm)
add_subdirectory(kio)
add_subdirectory(icon)
add_subdirectory(interfaces)

add_subdirectory(kded)
add_subdirectory(plugins)
add_subdirectory(plasmoid)
add_subdirectory(cli)
add_subdirectory(fileitemactionplugin)

add_subdirectory(tests)
