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

kdeconnect_add_plugin(kdeconnect_share JSON kdeconnect_share.json SOURCES ${kdeconnect_share_SRCS})

target_link_libraries(kdeconnect_share
    kdeconnectcore
    Qt5::DBus
    KF5::Notifications
    KF5::I18n
    KF5::KIOWidgets
    KF5::Service
)

#######################################
# Config

set( kdeconnect_share_config_SRCS share_config.cpp )
ki18n_wrap_ui( kdeconnect_share_config_SRCS share_config.ui )

add_library(kdeconnect_share_config MODULE ${kdeconnect_share_config_SRCS} )
target_link_libraries( kdeconnect_share_config
    kdeconnectpluginkcm
    KF5::I18n
    KF5::CoreAddons
    KF5::ConfigWidgets
    KF5::KIOWidgets
    KF5::Notifications
)

install(TARGETS kdeconnect_share_config DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES kdeconnect_share_config.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
