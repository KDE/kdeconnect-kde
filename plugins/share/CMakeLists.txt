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
    KF5::Notifications
    KF5::I18n
    KF5::GuiAddons
    KF5::KIOWidgets
    KF5::Service
)

#######################################
# Config

kcoreaddons_add_plugin(kdeconnect_share_config SOURCES share_config.cpp INSTALL_NAMESPACE "kdeconnect/kcms")

ki18n_wrap_ui(kdeconnect_share_config share_config.ui)
target_link_libraries( kdeconnect_share_config
    kdeconnectpluginkcm
    KF5::I18n
    KF5::CoreAddons
    KF5::ConfigWidgets
    KF5::KIOWidgets
    KF5::Notifications
)
