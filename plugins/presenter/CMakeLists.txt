qt_add_resources(presenter_SRCS assets.qrc)

kdeconnect_add_plugin(kdeconnect_presenter SOURCES presenterplugin.cpp ${presenter_SRCS})

target_link_libraries(kdeconnect_presenter
    kdeconnectcore
    Qt::DBus
    Qt::Quick
    KF6::I18n
)
