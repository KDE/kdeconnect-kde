set(debug_file_SRCS)
ecm_qt_declare_logging_category(
    debug_file_SRCS HEADER plugin_share_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_SHARE CATEGORY_NAME kdeconnect.plugin.share
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin share)")

set(kdeconnect_share_SRCS
    shareplugin.cpp
    ${debug_file_SRCS}
)

kdeconnect_add_plugin(kdeconnect_share SOURCES ${kdeconnect_share_SRCS})

target_link_libraries(kdeconnect_share
    kdeconnectcore
    Qt::DBus
    KF${QT_MAJOR_VERSION}::Notifications
    KF${QT_MAJOR_VERSION}::I18n
    KF${QT_MAJOR_VERSION}::GuiAddons
    KF${QT_MAJOR_VERSION}::KIOWidgets
    KF${QT_MAJOR_VERSION}::Service
)

#######################################
# Config

kcoreaddons_add_plugin(kdeconnect_share_config SOURCES share_config.cpp INSTALL_NAMESPACE "kdeconnect/kcms")

ki18n_wrap_ui(kdeconnect_share_config share_config.ui)
target_link_libraries( kdeconnect_share_config
    kdeconnectpluginkcm
    KF${QT_MAJOR_VERSION}::I18n
    KF${QT_MAJOR_VERSION}::CoreAddons
    KF${QT_MAJOR_VERSION}::ConfigWidgets
    KF${QT_MAJOR_VERSION}::KIOWidgets
    KF${QT_MAJOR_VERSION}::Notifications
)
