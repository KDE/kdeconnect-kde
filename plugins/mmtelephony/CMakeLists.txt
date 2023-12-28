kdeconnect_add_plugin(kdeconnect_mmtelephony SOURCES mmtelephonyplugin.cpp)
target_link_libraries(kdeconnect_mmtelephony
    kdeconnectcore
    Qt::DBus
    KF6::ModemManagerQt
    KF6::I18n
)
