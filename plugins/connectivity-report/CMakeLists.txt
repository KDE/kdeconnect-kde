kdeconnect_add_plugin(kdeconnect_connectivity_report SOURCES connectivity_reportplugin.cpp)
target_link_libraries(kdeconnect_connectivity_report
    kdeconnectcore
    Qt::DBus
    KF${QT_MAJOR_VERSION}::Solid
    KF${QT_MAJOR_VERSION}::I18n
)
