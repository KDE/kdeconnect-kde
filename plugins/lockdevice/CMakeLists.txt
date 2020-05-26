qt5_add_dbus_interface(lockdevice_SRCS org.freedesktop.ScreenSaver.xml screensaverdbusinterface)

set(debug_file_SRCS)
ecm_qt_declare_logging_category(
    debug_file_SRCS HEADER plugin_lock_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_LOCKREMOTE CATEGORY_NAME kdeconnect.plugin.lock
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin lockremote)")

kdeconnect_add_plugin(kdeconnect_lockdevice JSON kdeconnect_lockdevice.json SOURCES lockdeviceplugin.cpp ${lockdevice_SRCS} ${debug_file_SRCS})

target_link_libraries(kdeconnect_lockdevice
    kdeconnectcore
    Qt5::DBus
    KF5::I18n
)
