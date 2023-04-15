add_definitions(-DTRANSLATION_DOMAIN=\"kdeconnect-core\")

add_library(kdeconnectpluginkcm kdeconnectpluginkcm.cpp)
target_link_libraries(kdeconnectpluginkcm
PUBLIC
    kdeconnectcore
PRIVATE
    Qt::DBus
    Qt::Gui
    KF${QT_MAJOR_VERSION}::I18n
    KF${QT_MAJOR_VERSION}::ConfigCore
    KF${QT_MAJOR_VERSION}::KCMUtils
)

set_target_properties(kdeconnectpluginkcm PROPERTIES
    VERSION ${KDECONNECT_VERSION}
    SOVERSION ${KDECONNECT_VERSION_MAJOR}
)

generate_export_header(kdeconnectpluginkcm EXPORT_FILE_NAME kdeconnectpluginkcm_export.h BASE_NAME kdeconnectpluginkcm)

# Remove NAMELINK_SKIP if/when headers are being installed and the library becomes public.
install(TARGETS kdeconnectpluginkcm EXPORT kdeconnectLibraryTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)
