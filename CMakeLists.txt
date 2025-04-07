cmake_minimum_required(VERSION 3.16)

# KDE Release Service Version, managed by release script
set (RELEASE_SERVICE_VERSION_MAJOR "25")
set (RELEASE_SERVICE_VERSION_MINOR "04")
set (RELEASE_SERVICE_VERSION_MICRO "1")
set (RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")

project(kdeconnect VERSION ${RELEASE_SERVICE_VERSION})

set(KF_MIN_VERSION "6.0.0")
set(QT_MIN_VERSION "6.7.0")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
#set(CMAKE_COMPILE_WARNING_AS_ERROR TRUE)

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

include(KDEInstallDirs)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDECMakeSettings)
include(ECMAddTests)
include(ECMSetupVersion)
include(ECMInstallIcons)
include(ECMQtDeclareLoggingCategory)
include(FeatureSummary)
include(GenerateExportHeader)
include(ECMGenerateQmlTypes)
include(KDEClangFormat)
include(KDEGitCommitHooks)
include(ECMFindQmlModule)
include(ECMQmlModule)
include(ECMDeprecationSettings)

include(KDEConnectMacros.cmake)

ecm_setup_version(${RELEASE_SERVICE_VERSION}
    VARIABLE_PREFIX KDECONNECT
    VERSION_HEADER ${CMAKE_CURRENT_BINARY_DIR}/kdeconnect-version.h
)

ecm_set_disabled_deprecation_versions(
    QT 6.6
    KF 6.5
)

# Make the version header available by linking against kdeconnectversion
add_library(kdeconnectversion INTERFACE)
target_include_directories(kdeconnectversion INTERFACE ${CMAKE_CURRENT_BINARY_DIR})

set_package_properties(KF6Kirigami2 PROPERTIES
    DESCRIPTION "QtQuick plugins to build user interfaces based on KDE UX guidelines"
    PURPOSE "Required for KDE Connect's QML-based GUI applications"
    URL "https://www.kde.org/products/kirigami/"
    TYPE RUNTIME
)

find_package(KF6KirigamiAddons 0.11 REQUIRED)
set_package_properties(KF6KirigamiAddons PROPERTIES
    TYPE REQUIRED
    PURPOSE "Required runtime dependency (all the modules are required)"
)

if(UNIX AND NOT APPLE)
    option(WITH_X11 "Build with X11 support" ON)
    option(WITH_PULSEAUDIO "Build with Pulseaudio support" ON)
else()
    set(WITH_X11 OFF)
    set(WITH_PULSEAUDIO OFF)
endif()

if(UNIX AND NOT APPLE)
    find_package(KF6Package REQUIRED)
    if (WITH_PULSEAUDIO)
        find_package(KF6PulseAudioQt REQUIRED)
    endif()
    find_package(QtWaylandScanner REQUIRED)
    find_package(Wayland 1.9 REQUIRED Client)
    find_package(Qt6 REQUIRED COMPONENTS WaylandClient)
    find_package(WaylandProtocols REQUIRED)
    pkg_check_modules(XkbCommon IMPORTED_TARGET xkbcommon)
    find_package(PkgConfig QUIET REQUIRED)
    pkg_check_modules(DBus REQUIRED IMPORTED_TARGET dbus-1)
endif()

if (NOT WIN32)
    find_package(KF6StatusNotifierItem ${KF_MIN_VERSION} REQUIRED)
endif()

#ecm_find_qmlmodule(org.kde.people 1.0) FIXME This results in KPeople code being called which triggers DBus calls which breaks the Windows CI
ecm_find_qmlmodule(QtQuick.Particles 2.0)

add_definitions(-DQT_NO_URL_CAST_FROM_STRING -DQT_NO_KEYWORDS -DQT_NO_CAST_FROM_ASCII)

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS DBus Quick QuickWidgets QuickControls2 Network Multimedia)

if (Qt6Gui_VERSION VERSION_GREATER_EQUAL "6.10.0")
    find_package(Qt6GuiPrivate ${REQUIRED_QT_VERSION} REQUIRED NO_MODULE)
endif()

find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS I18n ConfigWidgets DBusAddons IconThemes Notifications
    KIO KCMUtils Service Solid Kirigami People WindowSystem GuiAddons Crash ItemModels)

find_package(KF6DocTools ${KF_MIN_VERSION})
set_package_properties(KF6DocTools PROPERTIES DESCRIPTION
    "Tools to generate documentation"
    TYPE OPTIONAL
)

if (WIN32)
    get_filename_component(WINDOWS_KITS_DIR
        "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots;KitsRoot10]" ABSOLUTE)
    file(READ "${WINDOWS_KITS_DIR}/SDKManifest.xml" SDK_MANIFEST)
    string(REGEX MATCH "PlatformIdentity[^\n]*" PLATFORM_IDENTITY "${SDK_MANIFEST}" )
    string(REGEX MATCH "[0-9][0-9][0-9][0-9][0-9]" WIN_SDK_VERSION "${PLATFORM_IDENTITY}" )
    add_compile_definitions(WIN_SDK_VERSION=${WIN_SDK_VERSION})
endif()

add_subdirectory(core)
add_subdirectory(plugins)
add_subdirectory(interfaces)

find_package(KF6QQC2DesktopStyle ${KF_MIN_VERSION} REQUIRED)
set_package_properties(KF6QQC2DesktopStyle PROPERTIES TYPE RUNTIME)

add_subdirectory(icons)
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
add_subdirectory(settings)

if(NOT WIN32 AND NOT APPLE)
    add_subdirectory(kio)
    add_subdirectory(plasmoid)
endif()

ki18n_install(po)
if(KF6DocTools_FOUND)
    kdoctools_install(po)
    add_subdirectory(doc)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

ecm_qt_install_logging_categories(EXPORT kdeconnect-kde FILE kdeconnect-kde.categories SORT DESTINATION "${KDE_INSTALL_LOGGINGCATEGORIESDIR}")

file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})

kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
