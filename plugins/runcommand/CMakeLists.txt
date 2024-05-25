kdeconnect_add_plugin(kdeconnect_runcommand SOURCES runcommandplugin.cpp)
target_link_libraries(kdeconnect_runcommand
    kdeconnectcore
    Qt::DBus
    KF6::KCMUtils
    KF6::I18n)

#----------------------
kdeconnect_add_kcm(kdeconnect_runcommand_config SOURCES runcommand_config.cpp)
target_link_libraries(kdeconnect_runcommand_config
    kdeconnectcore
    kdeconnectpluginkcm
    Qt::DBus
    KF6::I18n
    KF6::CoreAddons
    KF6::ConfigWidgets
)
