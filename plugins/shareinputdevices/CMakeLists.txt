kdeconnect_add_plugin(kdeconnect_shareinputdevices SOURCES shareinputdevicesplugin.cpp inputcapturesession.cpp)

pkg_check_modules(PKG_libei REQUIRED IMPORTED_TARGET libei-1.0)

target_link_libraries(kdeconnect_shareinputdevices
    kdeconnectcore
    kdeconnectinterfaces
    KF6::I18n
    Qt::GuiPrivate
    PkgConfig::PKG_libei
)

kdeconnect_add_kcm(kdeconnect_shareinputdevices_config SOURCES shareinputdevices_config.cpp)
ki18n_wrap_ui(kdeconnect_shareinputdevices_config shareinputdevices_config.ui)
target_link_libraries(kdeconnect_shareinputdevices_config
    kdeconnectpluginkcm
    kdeconnectinterfaces
    KF6::I18n
)
