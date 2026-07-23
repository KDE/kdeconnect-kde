kdeconnect_add_plugin(kdeconnect_shareinputdevices SOURCES shareinputdevicesplugin.cpp inputcapturesession.cpp)

target_link_libraries(kdeconnect_shareinputdevices
    kdeconnectcore
    kdeconnectdbusinterfaces
    KF6::I18n
    Qt::GuiPrivate
    PkgConfig::PKG_libei
)

install(FILES "kdeconnect_shareinputdevices_config.qml" DESTINATION ${KDE_INSTALL_DATADIR}/kdeconnect)
