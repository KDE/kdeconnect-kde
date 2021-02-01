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

kdeconnect_add_plugin(kdeconnect_notifications JSON kdeconnect_notifications.json SOURCES ${kdeconnect_notifications_SRCS})

target_link_libraries(kdeconnect_notifications
    kdeconnectcore
    Qt5::DBus
    KF5::Notifications
    KF5::I18n
    KF5::WindowSystem
)

if (UNIX AND NOT APPLE)
    target_link_libraries(kdeconnect_notifications
        Qt::X11Extras
    )
endif()
