kdeconnect_add_plugin(kdeconnect_connectivity_report SOURCES connectivity_reportplugin.cpp)
target_link_libraries(kdeconnect_connectivity_report
    kdeconnectcore
    Qt::DBus
    KF6::Solid
    KF6::I18n
)
