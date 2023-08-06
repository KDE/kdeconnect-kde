kdeconnect_add_plugin(kdeconnect_telephony SOURCES telephonyplugin.cpp)
target_link_libraries(kdeconnect_telephony
    kdeconnectcore
    KF${QT_MAJOR_VERSION}::I18n
    KF${QT_MAJOR_VERSION}::Notifications
    Qt::DBus
)
