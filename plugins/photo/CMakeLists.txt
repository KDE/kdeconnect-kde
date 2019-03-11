set(kdeconnect_photo_SRCS
    photoplugin.cpp
)

kdeconnect_add_plugin(kdeconnect_photo JSON kdeconnect_photo.json SOURCES ${kdeconnect_photo_SRCS})

target_link_libraries(kdeconnect_photo
    kdeconnectcore
    Qt5::DBus
)
