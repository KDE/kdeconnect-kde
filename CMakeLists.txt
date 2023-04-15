cmake_minimum_required(VERSION 3.16)

# KDE Release Service Version, managed by release script
set (RELEASE_SERVICE_VERSION_MAJOR "23")
set (RELEASE_SERVICE_VERSION_MINOR "07")
set (RELEASE_SERVICE_VERSION_MICRO "70")
set (RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")

project(kdeconnect VERSION ${RELEASE_SERVICE_VERSION})

if (SAILFISHOS)
    set(KF5_MIN_VERSION "5.36.0")
    set(QT_MIN_VERSION "5.6.0")
else()
    set(KF5_MIN_VERSION "5.101.0")
    set(QT_MIN_VERSION "5.15.2")
    set(KDE_COMPILERSETTINGS_LEVEL "5.84.0")
endif()
set(KDE_COMPILERSETTINGS_LEVEL "5.82")
set(QCA_MIN_VERSION "2.1.0")

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
include(ECMQmlModule)
if (NOT SAILFISHOS)
    include(ECMQMLModules)
endif()

include(KDEConnectMacros.cmake)

ecm_setup_version(${RELEASE_SERVICE_VERSION}
    VARIABLE_PREFIX KDECONNECT
    VERSION_HEADER ${CMAKE_CURRENT_BINARY_DIR}/kdeconnect-version.h
)

# Make the version header available by linking against kdeconnectversion
add_library(kdeconnectversion INTERFACE)
target_include_directories(kdeconnectversion INTERFACE ${CMAKE_CURRENT_BINARY_DIR})

if (SAILFISHOS)
    set(KF5_REQUIRED_COMPONENTS I18n CoreAddons Config)
    set(KF5_OPTIONAL_COMPONENTS)

    set(Qca_LIBRARY CONAN_PKG::Qca-qt5)

    add_definitions(-DSAILFISHOS)
else()
    find_package(Qca-qt${QT_MAJOR_VERSION} ${QCA_MIN_VERSION} REQUIRED)
    set(Qca_LIBRARY qca-qt${QT_MAJOR_VERSION})

    set(KF5_REQUIRED_COMPONENTS I18n ConfigWidgets DBusAddons IconThemes Notifications KIO KCMUtils Service Solid Kirigami2 People WindowSystem GuiAddons)
    set(KF5_OPTIONAL_COMPONENTS DocTools)

    set_package_properties(KF5Kirigami2 PROPERTIES
        DESCRIPTION "QtQuick plugins to build user interfaces based on KDE UX guidelines"
        PURPOSE "Required for KDE Connect's QML-based GUI applications"
        URL "https://www.kde.org/products/kirigami/"
        TYPE RUNTIME
    )

    if(UNIX AND NOT APPLE)
        find_package(KF5Package REQUIRED)
        find_package(KF5PulseAudioQt)
        if (QT_MAJOR_VERSION EQUAL "5")
            find_package(QtWaylandScanner REQUIRED)
        endif()
        find_package(Wayland 1.9 REQUIRED Client)
        find_package(Qt${QT_MAJOR_VERSION} REQUIRED COMPONENTS WaylandClient)
        find_package(WaylandProtocols REQUIRED)
        pkg_check_modules(XkbCommon IMPORTED_TARGET xkbcommon)
        pkg_check_modules(GIO REQUIRED IMPORTED_TARGET gio-2.0)
    endif()

    find_package(KF5PeopleVCard)
    set_package_properties(KF5PeopleVCard PROPERTIES
        PURPOSE "Read vcards from the file system"
        URL "https://invent.kde.org/pim/kpeoplevcard"
        TYPE RUNTIME
    )

    add_definitions(-DHAVE_KIO)

    #ecm_find_qmlmodule(org.kde.people 1.0) FIXME This results in KPeople code being called which triggers DBus calls which breaks the Windows CI
    ecm_find_qmlmodule(QtQuick.Particles 2.0)

endif()

add_definitions(-DQT_NO_URL_CAST_FROM_STRING -DQT_NO_KEYWORDS -DQT_NO_CAST_FROM_ASCII)

find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} REQUIRED COMPONENTS DBus Quick QuickControls2 Network Multimedia)
if (UNIX AND NOT APPLE AND QT_MAJOR_VERSION EQUAL "5")
    find_package(Qt5 ${QT_MIN_VERSION} REQUIRED COMPONENTS X11Extras)
endif()
find_package(KF${QT_MAJOR_VERSION} ${KF5_MIN_VERSION} REQUIRED COMPONENTS ${KF5_REQUIRED_COMPONENTS})
if (KF5_OPTIONAL_COMPONENTS)
    find_package(KF${QT_MAJOR_VERSION} ${KF5_MIN_VERSION} OPTIONAL_COMPONENTS ${KF5_OPTIONAL_COMPONENTS})
endif()

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

if (NOT SAILFISHOS)

    find_package(KF${QT_MAJOR_VERSION}QQC2DesktopStyle ${KF5_MIN_VERSION} REQUIRED)
    set_package_properties(KF${QT_MAJOR_VERSION}QQC2DesktopStyle PROPERTIES TYPE RUNTIME)

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
endif()

if(KF5DocTools_FOUND)
    add_subdirectory(doc)
    kdoctools_install(po)
endif()
ki18n_install(po)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

ecm_qt_install_logging_categories(EXPORT kdeconnect-kde FILE kdeconnect-kde.categories SORT DESTINATION "${KDE_INSTALL_LOGGINGCATEGORIESDIR}")

file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})

kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
