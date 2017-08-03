find_package(KF5 REQUIRED COMPONENTS Notifications KCMUtils I18n IconThemes Config)

set(kdeconnect_notifications_SRCS
    notification.cpp
    notificationsplugin.cpp
    notificationsdbusinterface.cpp
    sendreplydialog.cpp
)

ki18n_wrap_ui(kdeconnect_notifications_SRCS sendreplydialog.ui)

kdeconnect_add_plugin(kdeconnect_notifications JSON kdeconnect_notifications.json SOURCES ${kdeconnect_notifications_SRCS})

target_link_libraries(kdeconnect_notifications
    kdeconnectcore
    Qt5::DBus
    KF5::Notifications
    KF5::I18n
    KF5::IconThemes
)
