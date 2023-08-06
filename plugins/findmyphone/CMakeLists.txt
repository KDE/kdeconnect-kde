kdeconnect_add_plugin(kdeconnect_findmyphone SOURCES findmyphoneplugin.cpp)
target_link_libraries(kdeconnect_findmyphone kdeconnectcore Qt::Core Qt::DBus)

