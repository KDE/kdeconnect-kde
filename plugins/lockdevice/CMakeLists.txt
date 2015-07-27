qt5_add_dbus_interface(lockdevice_SRCS org.freedesktop.ScreenSaver.xml screensaverdbusinterface)

kdeconnect_add_plugin(kdeconnect_lockdevice JSON kdeconnect_lockdevice.json SOURCES lockdeviceplugin.cpp ${lockdevice_SRCS})

target_link_libraries(kdeconnect_lockdevice
    kdeconnectcore
    Qt5::DBus
    KF5::I18n
)
