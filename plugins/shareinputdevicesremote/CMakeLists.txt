kdeconnect_add_plugin(kdeconnect_shareinputdevicesremote SOURCES shareinputdevicesremoteplugin.cpp)

target_link_libraries(kdeconnect_shareinputdevicesremote
    kdeconnectcore
    Qt::Gui
    KF6::I18n
)
