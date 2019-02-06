cmake_minimum_required(VERSION 3.0)

project(kdeconnect)

set(KDECONNECT_VERSION_MAJOR 1)
set(KDECONNECT_VERSION_MINOR 3)
set(KDECONNECT_VERSION_PATCH 3)

set(KDECONNECT_VERSION "${KDECONNECT_VERSION_MAJOR}.${KDECONNECT_VERSION_MINOR}.${KDECONNECT_VERSION_PATCH}")

find_package(PkgConfig)

if (SAILFISHOS)
    set(KF5_MIN_VERSION "5.31.0")
    set(QT_MIN_VERSION "5.6.0")
    set(KF5_REQUIRED_COMPONENTS I18n DBusAddons CoreAddons IconThemes Config Service)
    set(KF5_OPTIONAL_COMPONENTS)
    set(QCA_MIN_VERSION 2.0.0)
    pkg_search_module(SFOS REQUIRED sailfishapp)
    pkg_check_modules(QCA2 qca2-qt5>=${QCA_MIN_VERSION} REQUIRED)
    add_definitions(-DSAILFISHOS)
    include_directories(${QCA2_INCLUDEDIR})
    add_definitions(-DQT_NO_URL_CAST_FROM_STRING)
else()
    set(KF5_MIN_VERSION "5.42.0")
    set(QT_MIN_VERSION "5.7.0")
    set(KF5_REQUIRED_COMPONENTS I18n ConfigWidgets DBusAddons IconThemes Notifications KIO KCMUtils Service)
    set(KF5_OPTIONAL_COMPONENTS DocTools)
    if(UNIX)
        set(KF5_OPTIONAL_COMPONENTS ${KF5_OPTIONAL_COMPONENTS} Runner)
    endif()
    set(QCA_MIN_VERSION "2.1.0")
    find_package(Qca-qt5 ${QCA_MIN_VERSION} REQUIRED)
    find_package(KF5PulseAudioQt)
    add_definitions(-DQT_NO_URL_CAST_FROM_STRING -DQT_NO_KEYWORDS)
endif()

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} ${CMAKE_SOURCE_DIR}/cmake)

find_package(Qt5 ${QT_MIN_VERSION} REQUIRED COMPONENTS Quick Network)
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS ${KF5_REQUIRED_COMPONENTS})
if (KF5_OPTIONAL_COMPONENTS)
find_package(KF5 ${KF5_MIN_VERSION} COMPONENTS ${KF5_OPTIONAL_COMPONENTS})
endif()

find_package(Phonon4Qt5 4.9.0 NO_MODULE)
set_package_properties(Phonon4Qt5 PROPERTIES
   DESCRIPTION "Qt-based audio library"
   TYPE OPTIONAL
   PURPOSE "Required for Find My Device plugin"
)

set_package_properties(KF5Kirigami2 PROPERTIES
  DESCRIPTION "QtQuick plugins to build user interfaces based on KDE UX guidelines"
  PURPOSE "Required for KDE Connect's QML-based GUI applications"
  URL "https://www.kde.org/products/kirigami/"
  TYPE RUNTIME
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

include(GenerateExportHeader)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(core)
if(NOT SAILFISHOS)
    add_subdirectory(kcm)
    add_subdirectory(kcmplugin)
    add_subdirectory(daemon)
endif()

if(NOT WIN32 AND NOT SAILFISHOS)
    add_subdirectory(kio)
    add_subdirectory(plasmoid)
endif()

add_subdirectory(icon)
add_subdirectory(interfaces)
add_subdirectory(data)

option(EXPERIMENTALAPP_ENABLED OFF)
if(EXPERIMENTALAPP_ENABLED)
  find_package(KF5Kirigami2)
  add_subdirectory(app)
endif()
add_subdirectory(plugins)
add_subdirectory(cli)
add_subdirectory(declarativeplugin)

if(KF5Runner_FOUND)
    add_subdirectory(runners)
endif()

if (NOT SAILFISHOS)
    add_subdirectory(indicator)
    add_subdirectory(urlhandler)
    add_subdirectory(nautilus-extension)
else()
    add_subdirectory(sfos)
endif()

option(SMSAPP_ENABLED OFF)
if(SMSAPP_ENABLED)
    find_package(KF5Kirigami2)
    find_package(KF5People REQUIRED)
    find_package(KF5PeopleVCard)
    set_package_properties(KF5PeopleVCard PROPERTIES
        PURPOSE "Read vcards from the file system"
        URL "https://phabricator.kde.org/source/kpeoplevcard/"
        TYPE RUNTIME
    )

    add_subdirectory(smsapp)
endif()

if(KF5DocTools_FOUND)
    add_subdirectory(doc)
endif()

if(BUILD_TESTING AND NOT SAILFISHOS)
    add_subdirectory(tests)
endif()

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
