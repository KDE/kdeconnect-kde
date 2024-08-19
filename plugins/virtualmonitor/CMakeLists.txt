kdeconnect_add_plugin(kdeconnect_virtualmonitor SOURCES virtualmonitorplugin.cpp)
target_link_libraries(kdeconnect_virtualmonitor
    kdeconnectcore
    Qt::Core
    Qt::Multimedia
    Qt::DBus
    KF6::I18n
)
