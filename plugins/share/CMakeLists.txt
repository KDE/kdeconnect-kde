kdeconnect_add_plugin(kdeconnect_share SOURCES shareplugin.cpp)
target_link_libraries(kdeconnect_share
    kdeconnectcore
    Qt::DBus
    KF${QT_MAJOR_VERSION}::Notifications
    KF${QT_MAJOR_VERSION}::I18n
    KF${QT_MAJOR_VERSION}::GuiAddons
    KF${QT_MAJOR_VERSION}::KIOWidgets
    KF${QT_MAJOR_VERSION}::Service
)

kdeconnect_add_kcm(kdeconnect_share_config SOURCES share_config.cpp)
ki18n_wrap_ui(kdeconnect_share_config share_config.ui)
target_link_libraries(kdeconnect_share_config
    kdeconnectpluginkcm
    KF${QT_MAJOR_VERSION}::I18n
    KF${QT_MAJOR_VERSION}::CoreAddons
    KF${QT_MAJOR_VERSION}::ConfigWidgets
    KF${QT_MAJOR_VERSION}::KIOWidgets
    KF${QT_MAJOR_VERSION}::Notifications
)
