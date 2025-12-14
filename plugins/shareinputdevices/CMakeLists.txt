kdeconnect_add_plugin(kdeconnect_shareinputdevices SOURCES shareinputdevicesplugin.cpp inputcapturesession.cpp)

pkg_check_modules(PKG_libei REQUIRED IMPORTED_TARGET libei-1.0)

target_link_libraries(kdeconnect_shareinputdevices
    kdeconnectcore
    kdeconnectdbusinterfaces
    KF6::I18n
    Qt::GuiPrivate
    PkgConfig::PKG_libei
)
