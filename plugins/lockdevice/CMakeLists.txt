kdeconnect_add_plugin(kdeconnect_lockdevice)

if (WIN32)
    target_sources(kdeconnect_lockdevice PRIVATE lockdeviceplugin-win.cpp)
else ()
    target_sources(kdeconnect_lockdevice PRIVATE lockdeviceplugin.cpp)
endif()
target_link_libraries(kdeconnect_lockdevice
    kdeconnectcore
    kdeconnectinterfaces
    Qt::DBus
    KF${QT_MAJOR_VERSION}::I18n
)
