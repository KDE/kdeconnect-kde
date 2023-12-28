kdeconnect_add_plugin(kdeconnect_remotesystemvolume SOURCES remotesystemvolumeplugin.cpp)
target_link_libraries(kdeconnect_remotesystemvolume
    kdeconnectcore
    Qt::DBus
    KF6::I18n
)
