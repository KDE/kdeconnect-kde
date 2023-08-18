kdeconnect_add_plugin(kdeconnect_notifications SOURCES sendreplydialog.cpp notificationsplugin.cpp notification.cpp)

ki18n_wrap_ui(kdeconnect_notifications sendreplydialog.ui)
target_link_libraries(kdeconnect_notifications
    kdeconnectcore
    Qt::DBus
    Qt::Widgets
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
