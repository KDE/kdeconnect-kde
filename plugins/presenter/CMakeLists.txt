qt5_add_resources(presenter_SRCS assets.qrc)

kdeconnect_add_plugin(kdeconnect_presenter JSON kdeconnect_presenter.json SOURCES presenterplugin.cpp ${presenter_SRCS})
target_link_libraries(kdeconnect_presenter
    kdeconnectcore
    Qt5::DBus
    Qt5::Quick
    KF5::I18n
)
