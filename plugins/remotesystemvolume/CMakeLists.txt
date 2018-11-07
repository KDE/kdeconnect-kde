set(kdeconnect_remotesystemvolume_SRCS
    remotesystemvolumeplugin.cpp
)

kdeconnect_add_plugin(kdeconnect_remotesystemvolume JSON kdeconnect_remotesystemvolume.json SOURCES ${kdeconnect_remotesystemvolume_SRCS})

target_link_libraries(kdeconnect_remotesystemvolume
    kdeconnectcore
    Qt5::DBus
    KF5::I18n
)
