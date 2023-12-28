kdeconnect_add_plugin(kdeconnect_share SOURCES shareplugin.cpp)
target_link_libraries(kdeconnect_share
    kdeconnectcore
    Qt::DBus
    KF6::Notifications
    KF6::I18n
    KF6::GuiAddons
    KF6::KIOWidgets
    KF6::Service
)

kdeconnect_add_kcm(kdeconnect_share_config SOURCES share_config.cpp)
ki18n_wrap_ui(kdeconnect_share_config share_config.ui)
target_link_libraries(kdeconnect_share_config
    kdeconnectpluginkcm
    KF6::I18n
    KF6::CoreAddons
    KF6::ConfigWidgets
    KF6::KIOWidgets
    KF6::Notifications
)
