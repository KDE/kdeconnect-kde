kdeconnect_add_plugin(kdeconnect_telephony SOURCES telephonyplugin.cpp)
target_link_libraries(kdeconnect_telephony
    kdeconnectcore
    KF6::I18n
    KF6::Notifications
    Qt::DBus
)
