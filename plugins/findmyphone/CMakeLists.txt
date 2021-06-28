set(kdeconnect_findmyphone_SRCS
    findmyphoneplugin.cpp
)

kdeconnect_add_plugin(kdeconnect_findmyphone SOURCES ${kdeconnect_findmyphone_SRCS})

target_link_libraries(kdeconnect_findmyphone kdeconnectcore Qt5::Core Qt5::DBus)


