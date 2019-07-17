cmake_minimum_required(VERSION 3.0)

project(kdeconnect)

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

include(KDEInstallDirs)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDECMakeSettings)
include(ECMAddTests)
include(ECMSetupVersion)
include(ECMInstallIcons)
include(FeatureSummary)
include(ECMQtDeclareLoggingCategory)
include(GenerateExportHeader)

include(KDEConnectMacros.cmake)

ecm_setup_version(1.3.3
    VARIABLE_PREFIX KDECONNECT
    VERSION_HEADER ${CMAKE_CURRENT_BINARY_DIR}/kdeconnect-version.h
)

if (SAILFISHOS)
    find_package(PkgConfig)
    set(KF5_MIN_VERSION "5.31.0")
    set(QT_MIN_VERSION "5.6.0")
    set(KF5_REQUIRED_COMPONENTS I18n DBusAddons CoreAddons IconThemes Config)
    set(KF5_OPTIONAL_COMPONENTS)
    set(QCA_MIN_VERSION 2.0.0)
    pkg_search_module(SFOS REQUIRED sailfishapp)
    pkg_check_modules(QCA2 qca2-qt5>=${QCA_MIN_VERSION} REQUIRED)
    add_definitions(-DSAILFISHOS)
    include_directories(${QCA2_INCLUDEDIR})
    add_definitions(-DQT_NO_URL_CAST_FROM_STRING)
else()
    set(KF5_MIN_VERSION "5.48.0")
    set(QT_MIN_VERSION "5.10.0")
    set(KF5_REQUIRED_COMPONENTS I18n ConfigWidgets DBusAddons IconThemes Notifications KIO KCMUtils Service)
    set(KF5_OPTIONAL_COMPONENTS DocTools)
    if(UNIX)
        set(KF5_OPTIONAL_COMPONENTS ${KF5_OPTIONAL_COMPONENTS} Runner)
    endif()
    set(QCA_MIN_VERSION "2.1.0")
    find_package(Qca-qt5 ${QCA_MIN_VERSION} REQUIRED)
    if(NOT WIN32 AND NOT APPLE)
        find_package(KF5PulseAudioQt REQUIRED)
    endif()
    add_definitions(-DQT_NO_URL_CAST_FROM_STRING -DQT_NO_KEYWORDS -DQT_NO_CAST_FROM_ASCII)
endif()

find_package(Qt5 ${QT_MIN_VERSION} REQUIRED COMPONENTS Quick Network)
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS ${KF5_REQUIRED_COMPONENTS})
if (KF5_OPTIONAL_COMPONENTS)
find_package(KF5 ${KF5_MIN_VERSION} COMPONENTS ${KF5_OPTIONAL_COMPONENTS})
endif()

if (NOT ZSH_AUTOCOMPLETE_DIR)
set(ZSH_AUTOCOMPLETE_DIR "${CMAKE_INSTALL_PREFIX}/share/zsh/site-functions")
endif()

find_package(Qt5Multimedia)

set_package_properties(KF5Kirigami2 PROPERTIES
  DESCRIPTION "QtQuick plugins to build user interfaces based on KDE UX guidelines"
  PURPOSE "Required for KDE Connect's QML-based GUI applications"
  URL "https://www.kde.org/products/kirigami/"
  TYPE RUNTIME
)

option(PRIVATE_DBUS_ENABLED "Use private dbus session for kdeconnect" OFF)
if(PRIVATE_DBUS_ENABLED OR APPLE)
    add_compile_definitions(USE_PRIVATE_DBUS)
endif()
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
    add_subdirectory(fileitemactionplugin)
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
