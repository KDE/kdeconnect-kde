set(kdeconnect_bigscreen_SRCS
    bigscreenplugin.cpp
)

kdeconnect_add_plugin(kdeconnect_bigscreen SOURCES ${kdeconnect_bigscreen_SRCS})

target_link_libraries(kdeconnect_bigscreen kdeconnectcore Qt${QT_MAJOR_VERSION}::DBus KF5::I18n)
