kdeconnect_add_plugin(kdeconnect_sendnotifications)

target_sources(kdeconnect_sendnotifications PRIVATE
    sendnotificationsplugin.cpp
    notificationslistener.cpp
    notifyingapplication.cpp
)

target_link_libraries(kdeconnect_sendnotifications
    kdeconnectcore
    KF6::I18n
    Qt::Gui
    KF6::IconThemes
    KF6::ConfigCore
)


if(WIN32)
target_sources(kdeconnect_sendnotifications PRIVATE windowsnotificationslistener.cpp)
target_link_libraries(kdeconnect_sendnotifications runtimeobject windowsapp)
else()
target_sources(kdeconnect_sendnotifications PRIVATE dbusnotificationslistener.cpp)
target_link_libraries(kdeconnect_sendnotifications PkgConfig::DBus)
endif()

install(FILES "kdeconnect_sendnotifications_config.qml" DESTINATION ${KDE_INSTALL_DATADIR}/kdeconnect)
