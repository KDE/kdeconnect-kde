qt_add_dbus_interface(lockdevice_SRCS org.freedesktop.login1.xml login1dbusinterface)
qt_add_dbus_interface(lockdevice_SRCS org.freedesktop.DBus.Properties.xml propertiesdbusinterface)

set(debug_file_SRCS)
ecm_qt_declare_logging_category(
    debug_file_SRCS HEADER plugin_lock_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_LOCKREMOTE CATEGORY_NAME kdeconnect.plugin.lock
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin lockremote)")

if (WIN32)
    list(APPEND lockdevice_SRCS lockdeviceplugin-win.cpp)
else ()
    list(APPEND lockdevice_SRCS lockdeviceplugin.cpp)
endif()

kdeconnect_add_plugin(kdeconnect_lockdevice SOURCES ${lockdevice_SRCS} ${debug_file_SRCS})

target_link_libraries(kdeconnect_lockdevice
    kdeconnectcore
    Qt::DBus
    KF${QT_MAJOR_VERSION}::I18n
)
