project(kdeconnectpluginkcm)

add_definitions(-DTRANSLATION_DOMAIN=\"kdeconnect-core\")

find_package(KF5 REQUIRED COMPONENTS KCMUtils)

add_library(kdeconnectpluginkcm kdeconnectpluginkcm.cpp)
target_link_libraries(kdeconnectpluginkcm
PUBLIC
    kdeconnectcore
PRIVATE
    Qt5::DBus
    Qt5::Gui
    KF5::I18n
    KF5::ConfigCore
    KF5::KCMUtils
)

set_target_properties(kdeconnectpluginkcm PROPERTIES
    VERSION ${KDECONNECT_VERSION}
    SOVERSION ${KDECONNECT_VERSION_MAJOR}
)

target_include_directories(kdeconnectpluginkcm PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
generate_export_header(kdeconnectpluginkcm EXPORT_FILE_NAME kdeconnectpluginkcm_export.h BASE_NAME kdeconnectpluginkcm)

# Remove NAMELINK_SKIP if/when headers are being installed and the library becomes public.
install(TARGETS kdeconnectpluginkcm EXPORT kdeconnectLibraryTargets ${INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)
