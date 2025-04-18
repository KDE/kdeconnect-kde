kdeconnect_add_plugin(kdeconnect_runcommand SOURCES runcommandplugin.cpp)
target_link_libraries(kdeconnect_runcommand
    kdeconnectcore
    Qt::DBus
    KF6::I18n)

install(FILES "kdeconnect_runcommand_config.qml" DESTINATION ${KDE_INSTALL_DATADIR}/kdeconnect)
