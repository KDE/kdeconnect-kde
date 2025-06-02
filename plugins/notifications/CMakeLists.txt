kdeconnect_add_plugin(kdeconnect_notifications SOURCES sendreplydialog.cpp notificationsplugin.cpp notification.cpp)

ki18n_wrap_ui(kdeconnect_notifications sendreplydialog.ui)
target_link_libraries(kdeconnect_notifications
    kdeconnectcore
    Qt::DBus
    Qt::Widgets
    KF6::GuiAddons
    KF6::I18n
    KF6::Notifications
    KF6::WindowSystem
)

if (UNIX AND NOT APPLE)
    target_link_libraries(kdeconnect_notifications Qt6::GuiPrivate)
endif()
