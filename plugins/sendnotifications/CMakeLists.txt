if (SAILFISHOS)
	set(SN_PLUGIN_ENABLED_BY_DEFAULT "true")
else()
	set(SN_PLUGIN_ENABLED_BY_DEFAULT "false")
endif()

configure_file(kdeconnect_sendnotifications.json.in kdeconnect_sendnotifications.json)

set(debug_file_SRCS)
ecm_qt_declare_logging_category(
    debug_file_SRCS HEADER plugin_sendnotification_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_SENDNOTIFICATION CATEGORY_NAME kdeconnect.plugin.sendnotification
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin sendnotification)")

set(kdeconnect_sendnotifications_SRCS
    sendnotificationsplugin.cpp
    notificationslistener.cpp
    notifyingapplication.cpp
    kdeconnect_sendnotifications.json
    ${debug_file_SRCS}
)

kdeconnect_add_plugin(kdeconnect_sendnotifications SOURCES ${kdeconnect_sendnotifications_SRCS})

target_link_libraries(kdeconnect_sendnotifications
    kdeconnectcore
    Qt::DBus
    KF${QT_MAJOR_VERSION}::I18n
    Qt::Gui
    KF${QT_MAJOR_VERSION}::IconThemes
    KF${QT_MAJOR_VERSION}::ConfigCore
    PkgConfig::GIO
)

#######################################
# Config
if (NOT SAILFISHOS)

kcoreaddons_add_plugin(kdeconnect_sendnotifications_config INSTALL_NAMESPACE "kdeconnect/kcms")

ki18n_wrap_ui(kdeconnect_sendnotifications_config sendnotifications_config.ui)
target_sources(kdeconnect_sendnotifications_config PRIVATE
     sendnotifications_config.cpp
     notifyingapplication.cpp
     notifyingapplicationmodel.cpp
)

target_link_libraries( kdeconnect_sendnotifications_config
    kdeconnectcore
    kdeconnectpluginkcm
    KF${QT_MAJOR_VERSION}::I18n
    KF${QT_MAJOR_VERSION}::KCMUtils
)
endif()
