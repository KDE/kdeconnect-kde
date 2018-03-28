cmake_minimum_required(VERSION 3.0)

project(kdeconnect)

set(KDECONNECT_VERSION_MAJOR 1)
set(KDECONNECT_VERSION_MINOR 3)
set(KDECONNECT_VERSION_PATCH 0)
set(KDECONNECT_VERSION "${KDECONNECT_VERSION_MAJOR}.${KDECONNECT_VERSION_MINOR}.${KDECONNECT_VERSION_PATCH}")

set(QT_MIN_VERSION "5.7.0")
set(KF5_MIN_VERSION "5.42.0")

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} ${CMAKE_SOURCE_DIR}/cmake)

find_package(Qt5 ${QT_MIN_VERSION} REQUIRED COMPONENTS Quick)

find_package(KF5 ${KF5_MIN_VERSION}
    REQUIRED COMPONENTS I18n ConfigWidgets DBusAddons IconThemes Notifications KIO KCMUtils
    OPTIONAL_COMPONENTS DocTools
)
find_package(Qca-qt5 2.1.0 REQUIRED)
find_package(Phonon4Qt5 4.9.0 NO_MODULE)
set_package_properties(Phonon4Qt5 PROPERTIES
   DESCRIPTION "Qt-based audio library"
   TYPE OPTIONAL
   PURPOSE "Required for Find My Device plugin"
)

include_directories(${CMAKE_SOURCE_DIR})

configure_file(kdeconnect-version.h.in ${CMAKE_CURRENT_BINARY_DIR}/kdeconnect-version.h)

include(KDEInstallDirs)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDECMakeSettings)
include(ECMAddTests)
include(ECMSetupVersion)
include(ECMInstallIcons)
include(FeatureSummary)

include(KDEConnectMacros.cmake)

add_definitions(-DQT_NO_URL_CAST_FROM_STRING -DQT_NO_KEYWORDS)

include(GenerateExportHeader)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(core)
add_subdirectory(kcm)
add_subdirectory(kcmplugin)
if(NOT WIN32)
	add_subdirectory(kio)
	add_subdirectory(plasmoid)
endif()
add_subdirectory(icon)
add_subdirectory(interfaces)

option(EXPERIMENTALAPP_ENABLED OFF)
if(EXPERIMENTALAPP_ENABLED)
    add_subdirectory(app)
endif()
add_subdirectory(daemon)
add_subdirectory(plugins)
add_subdirectory(cli)
add_subdirectory(indicator)
add_subdirectory(fileitemactionplugin)
add_subdirectory(urlhandler)
add_subdirectory(nautilus-extension)

option(SMSAPP_ENABLED OFF)
if(SMSAPP_ENABLED)
    find_package(KF5People REQUIRED)
    add_subdirectory(smsapp)
endif()
if(KF5DocTools_FOUND)
    add_subdirectory(doc)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

install(FILES org.kde.kdeconnect.kcm.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
