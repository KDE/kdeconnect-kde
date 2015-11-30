set(kdeconnect_findmyphone_SRCS
    findmyphoneplugin.cpp
)

kdeconnect_add_plugin(kdeconnect_findmyphone JSON kdeconnect_findmyphone.json SOURCES ${kdeconnect_findmyphone_SRCS})

target_link_libraries(kdeconnect_findmyphone kdeconnectcore kdeconnectcore Qt5::Core Qt5::DBus)


