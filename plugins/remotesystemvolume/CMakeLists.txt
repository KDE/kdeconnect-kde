kdeconnect_add_plugin(kdeconnect_remotesystemvolume SOURCES remotesystemvolumeplugin.cpp)
target_link_libraries(kdeconnect_remotesystemvolume
    kdeconnectcore
    Qt::DBus
    KF${QT_MAJOR_VERSION}::I18n
)
