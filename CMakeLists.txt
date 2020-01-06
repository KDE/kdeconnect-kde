cmake_minimum_required(VERSION 3.0)

project(kdeconnect)

if (SAILFISHOS)
    set(KF5_MIN_VERSION "5.36.0")
    set(QT_MIN_VERSION "5.6.0")
else()
    set(KF5_MIN_VERSION "5.64.0")
    set(QT_MIN_VERSION "5.10.0")
endif()
set(QCA_MIN_VERSION "2.1.0")

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

include(KDEInstallDirs)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDECMakeSettings)
include(ECMAddTests)
include(ECMSetupVersion)
include(ECMInstallIcons)
include(FeatureSummary)
include(GenerateExportHeader)

include(KDEConnectMacros.cmake)

ecm_setup_version(1.4.0
    VARIABLE_PREFIX KDECONNECT
    VERSION_HEADER ${CMAKE_CURRENT_BINARY_DIR}/kdeconnect-version.h
)

if (SAILFISHOS)
    set(KF5_REQUIRED_COMPONENTS I18n CoreAddons Config)
    set(KF5_OPTIONAL_COMPONENTS)
    
    set(Qca_LIBRARY CONAN_PKG::Qca-qt5)

    add_definitions(-DSAILFISHOS)
else()
    find_package(Qca-qt5 ${QCA_MIN_VERSION} REQUIRED)
    set(Qca_LIBRARY qca-qt5)

    set(KF5_REQUIRED_COMPONENTS I18n ConfigWidgets DBusAddons IconThemes Notifications KIO KCMUtils Service Kirigami2 People)
    set(KF5_OPTIONAL_COMPONENTS DocTools)

    set_package_properties(KF5Kirigami2 PROPERTIES
        DESCRIPTION "QtQuick plugins to build user interfaces based on KDE UX guidelines"
        PURPOSE "Required for KDE Connect's QML-based GUI applications"
        URL "https://www.kde.org/products/kirigami/"
        TYPE RUNTIME
    )

    if(UNIX AND NOT APPLE)
        find_package(KF5PulseAudioQt)
    endif()

    find_package(KF5PeopleVCard)
    set_package_properties(KF5PeopleVCard PROPERTIES
        PURPOSE "Read vcards from the file system"
        URL "https://phabricator.kde.org/source/kpeoplevcard/"
        TYPE RUNTIME
    )

    add_definitions(-DHAVE_KIO)
endif()

add_definitions(-DQT_NO_URL_CAST_FROM_STRING -DQT_NO_KEYWORDS -DQT_NO_CAST_FROM_ASCII)

find_package(Qt5 ${QT_MIN_VERSION} REQUIRED COMPONENTS DBus Quick Network Multimedia)
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS ${KF5_REQUIRED_COMPONENTS})
if (KF5_OPTIONAL_COMPONENTS)
    find_package(KF5 ${KF5_MIN_VERSION} COMPONENTS ${KF5_OPTIONAL_COMPONENTS})
endif()

if (NOT ZSH_AUTOCOMPLETE_DIR)
set(ZSH_AUTOCOMPLETE_DIR "${CMAKE_INSTALL_PREFIX}/share/zsh/site-functions")
endif()

option(PRIVATE_DBUS_ENABLED "Use private dbus session for kdeconnect" OFF)
if(PRIVATE_DBUS_ENABLED OR APPLE)
    add_compile_definitions(USE_PRIVATE_DBUS)
endif()

add_subdirectory(core)
add_subdirectory(plugins)
add_subdirectory(interfaces)

if (NOT SAILFISHOS)
    add_subdirectory(icon)
    add_subdirectory(data)
    add_subdirectory(cli)
    add_subdirectory(declarativeplugin)

    add_subdirectory(kcm)
    add_subdirectory(kcmplugin)
    add_subdirectory(daemon)
    add_subdirectory(app)
    add_subdirectory(indicator)
    add_subdirectory(urlhandler)
    add_subdirectory(nautilus-extension)
    add_subdirectory(fileitemactionplugin)
    add_subdirectory(smsapp)
    
    if(NOT WIN32)
        add_subdirectory(kio)
        add_subdirectory(plasmoid)
    endif()
endif()
    
if(KF5DocTools_FOUND)
    add_subdirectory(doc)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
