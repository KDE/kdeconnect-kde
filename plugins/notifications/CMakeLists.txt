kdeconnect_add_plugin(kdeconnect_notifications SOURCES sendreplydialog.cpp notificationsplugin.cpp notification.cpp)

ki18n_wrap_ui(kdeconnect_notifications sendreplydialog.ui)
ecm_qt_declare_logging_category(kdeconnect_notifications
    HEADER plugin_notification_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_NOTIFICATION CATEGORY_NAME kdeconnect.plugin.notification
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin notification)")

target_link_libraries(kdeconnect_notifications
    kdeconnectcore
    Qt::DBus
    KF${QT_MAJOR_VERSION}::Notifications
    KF${QT_MAJOR_VERSION}::I18n
    KF${QT_MAJOR_VERSION}::WindowSystem
)

if (UNIX AND NOT APPLE)
    if (QT_MAJOR_VERSION EQUAL "5")
        target_link_libraries(kdeconnect_notifications Qt5::X11Extras)
    else()
        target_link_libraries(kdeconnect_notifications Qt6::GuiPrivate)
    endif()
endif()
