set(debug_file_SRCS)
ecm_qt_declare_logging_category(
    debug_file_SRCS HEADER plugin_notification_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_NOTIFICATION CATEGORY_NAME kdeconnect.plugin.notification
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin notification)")

set(kdeconnect_notifications_SRCS
    notification.cpp
    notificationsplugin.cpp
    sendreplydialog.cpp
    ${debug_file_SRCS}
)

ki18n_wrap_ui(kdeconnect_notifications_SRCS sendreplydialog.ui)

kdeconnect_add_plugin(kdeconnect_notifications SOURCES ${kdeconnect_notifications_SRCS})

target_link_libraries(kdeconnect_notifications
    kdeconnectcore
    Qt::DBus
    KF${QT_MAJOR_VERSION}::Notifications
    KF${QT_MAJOR_VERSION}::I18n
    KF${QT_MAJOR_VERSION}::WindowSystem
)

if (UNIX AND NOT APPLE AND QT_MAJOR_VERSION EQUAL "5")
    target_link_libraries(kdeconnect_notifications
        Qt5::X11Extras
    )
endif()
