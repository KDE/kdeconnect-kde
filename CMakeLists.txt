cmake_minimum_required(VERSION 3.16)

# KDE Release Service Version, managed by release script
set (RELEASE_SERVICE_VERSION_MAJOR "26")
set (RELEASE_SERVICE_VERSION_MINOR "11")
set (RELEASE_SERVICE_VERSION_MICRO "70")
set (RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")

project(kdeconnect VERSION ${RELEASE_SERVICE_VERSION})

set(KF_MIN_VERSION "6.0.0")
set(QT_MIN_VERSION "6.7.0")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
#set(CMAKE_COMPILE_WARNING_AS_ERROR TRUE)

if(CMAKE_VERSION VERSION_LESS "3.25.0")
    if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
        set(LINUX ON)
    else()
        set(LINUX OFF)
    endif()
endif()

if(APPLE)
    # The default install paths would create one bundle per executable, but we want a single app bundle
    set(KDECONNECT_INSTALL_TARGETS_DEFAULT_ARGS
        RUNTIME DESTINATION "Applications/KDE/KDE Connect.app/Contents/MacOS"
        LIBRARY DESTINATION "Applications/KDE/KDE Connect.app/Contents/Frameworks"
        ARCHIVE DESTINATION "Applications/KDE/KDE Connect.app/Contents/Frameworks"
        OBJECTS DESTINATION "Applications/KDE/KDE Connect.app/Contents/Frameworks"
    )
    set(KDECONNECT_INSTALL_TARGETS_DEFAULT_ARGS_BUNDLE
        BUNDLE DESTINATION "Applications/KDE/"
    )
else()
    set(KDECONNECT_INSTALL_TARGETS_DEFAULT_ARGS
        "${KDE_INSTALL_TARGETS_DEFAULT_ARGS}"
    )
    set(KDECONNECT_INSTALL_TARGETS_DEFAULT_ARGS_BUNDLE
        "${KDE_INSTALL_TARGETS_DEFAULT_ARGS}"
    )
endif()

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

add_definitions(-DQT_NO_URL_CAST_FROM_STRING -DQT_NO_KEYWORDS -DQT_NO_CAST_FROM_ASCII -DQT_NO_CONTEXTLESS_CONNECT)

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS DBus Gui Multimedia Network Quick QuickWidgets QuickControls2)

if (Qt6Gui_VERSION VERSION_GREATER_EQUAL "6.10.0")
    find_package(Qt6GuiPrivate ${QT_MIN_VERSION} REQUIRED NO_MODULE)
endif()

option(BLUETOOTH_ENABLED "Bluetooth support for kdeconnect" ON)
if(BLUETOOTH_ENABLED)
    find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Bluetooth)
endif()

find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS ColorScheme Crash DBusAddons GuiAddons I18n
    IconThemes ItemModels KIO Kirigami Notifications People Service Solid WidgetsAddons WindowSystem)

find_package(KF6KirigamiAddons 0.11 REQUIRED)
set_package_properties(KF6KirigamiAddons PROPERTIES
    TYPE REQUIRED
    PURPOSE "Required runtime dependency (all the modules are required)"
)

find_package(KF6QQC2DesktopStyle ${KF_MIN_VERSION} REQUIRED)
set_package_properties(KF6QQC2DesktopStyle PROPERTIES TYPE RUNTIME)

#ecm_find_qmlmodule(org.kde.people 1.0) FIXME This results in KPeople code being called which triggers DBus calls which breaks the Windows CI
ecm_find_qmlmodule(QtQuick.Particles 2.0)

if(UNIX AND NOT APPLE)
    option(WITH_X11 "Build with X11 support" ON)
    option(WITH_PULSEAUDIO "Build with Pulseaudio support" ON)

    option(INSTALL_UFW_APPLICATION_RULE "Install application rule for ufw (firewall)" ON)
    add_feature_info("INSTALL_UFW_APPLICATION_RULE"
                 INSTALL_UFW_APPLICATION_RULE
                 "Install application rule for ufw (firewall)")

    find_package(KF6Package ${KF_MIN_VERSION} REQUIRED)
    if (WITH_PULSEAUDIO)
        find_package(KF6PulseAudioQt REQUIRED)
    endif()
    find_package(QtWaylandScanner REQUIRED)
    find_package(Wayland 1.9 REQUIRED Client)
    find_package(Qt6WaylandClient REQUIRED)
    find_package(WaylandProtocols REQUIRED)
    find_package(PkgConfig QUIET REQUIRED)
    pkg_check_modules(DBus REQUIRED IMPORTED_TARGET dbus-1)
    pkg_check_modules(PKG_libei REQUIRED IMPORTED_TARGET libei-1.0) # mousepad, shareinputdevices plugins
    pkg_check_modules(XkbCommon IMPORTED_TARGET xkbcommon) # plugins/mousepad
    if (WITH_X11)
        find_package(LibFakeKey REQUIRED)
        set_package_properties(LibFakeKey PROPERTIES DESCRIPTION "fake key events"
                            URL "https://github.com/dtapuska/fakekey"
                            TYPE REQUIRED
                            PURPOSE "Needed for the remote mouse input plugin"
                            )

        find_package(XTest REQUIRED)
        find_package(X11 REQUIRED)
    endif()
else()
    set(WITH_X11 OFF)
    set(WITH_PULSEAUDIO OFF)
    set(INSTALL_UFW_APPLICATION_RULE OFF)
endif()

if(LINUX)
    find_package(KF6ModemManagerQt ${KF_MIN_VERSION}) # plugins/mmtelephony
    pkg_check_modules(Evdev IMPORTED_TARGET libevdev) # plugins/digitizer
    if(TARGET PkgConfig::Evdev)
        message(STATUS "digitizer: Dependencies met, compiling.")
    else()
        message(WARNING "digitizer: Dependencies missing, not compiling.")
    endif()
endif()

if (NOT WIN32)
    find_package(KF6StatusNotifierItem ${KF_MIN_VERSION} REQUIRED)
else()
    get_filename_component(WINDOWS_KITS_DIR
        "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots;KitsRoot10]" ABSOLUTE)
    file(READ "${WINDOWS_KITS_DIR}/SDKManifest.xml" SDK_MANIFEST)
    string(REGEX MATCH "PlatformIdentity[^\n]*" PLATFORM_IDENTITY "${SDK_MANIFEST}" )
    string(REGEX MATCH "[0-9][0-9][0-9][0-9][0-9]" WIN_SDK_VERSION "${PLATFORM_IDENTITY}" )
    add_compile_definitions(WIN_SDK_VERSION=${WIN_SDK_VERSION})
endif()

find_package(OpenSSL REQUIRED COMPONENTS Crypto)

configure_file(config-X11.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-X11.h)

add_subdirectory(core)
add_subdirectory(plugins)
add_subdirectory(dbusinterfaces)
add_subdirectory(models)

add_subdirectory(icons)
add_subdirectory(data)
add_subdirectory(cli)
add_subdirectory(declarativeplugin)
add_subdirectory(daemon)
add_subdirectory(app)
add_subdirectory(indicator)
add_subdirectory(urlhandler)
add_subdirectory(nautilus-extension)
add_subdirectory(fileitemactionplugin)
add_subdirectory(smsapp)

if(UNIX AND NOT APPLE)
    add_subdirectory(kio)
    add_subdirectory(plasmoid)
endif()

ki18n_install(po)

if(BUILD_TESTING)
    add_compile_definitions(BUILD_TESTING)
    add_subdirectory(tests)
endif()

ecm_qt_install_logging_categories(EXPORT kdeconnect-kde FILE kdeconnect-kde.categories SORT DESTINATION "${KDE_INSTALL_LOGGINGCATEGORIESDIR}")

file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})

kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
