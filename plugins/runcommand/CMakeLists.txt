kdeconnect_add_plugin(kdeconnect_runcommand SOURCES runcommandplugin.cpp)
target_link_libraries(kdeconnect_runcommand
    kdeconnectcore
    Qt::DBus
    KF${QT_MAJOR_VERSION}::KCMUtils
    KF${QT_MAJOR_VERSION}::I18n)

#----------------------
kdeconnect_add_kcm(kdeconnect_runcommand_config SOURCES runcommand_config.cpp)
target_link_libraries(kdeconnect_runcommand_config
    kdeconnectcore
    kdeconnectpluginkcm
    KF${QT_MAJOR_VERSION}::I18n
    KF${QT_MAJOR_VERSION}::CoreAddons
    KF${QT_MAJOR_VERSION}::ConfigWidgets
)
