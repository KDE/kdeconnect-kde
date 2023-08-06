kdeconnect_add_plugin(kdeconnect_mmtelephony SOURCES mmtelephonyplugin.cpp)
target_link_libraries(kdeconnect_mmtelephony
    kdeconnectcore
    Qt::DBus
    KF${QT_MAJOR_VERSION}::ModemManagerQt
    KF${QT_MAJOR_VERSION}::I18n
)
