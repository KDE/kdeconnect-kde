set(kdeconnect_systemvolume_SRCS
    systemvolumeplugin.cpp
)

kdeconnect_add_plugin(kdeconnect_systemvolume JSON kdeconnect_systemvolume.json SOURCES ${kdeconnect_systemvolume_SRCS})

target_link_libraries(kdeconnect_systemvolume
    kdeconnectcore
    Qt5::Core
    KF5::PulseAudioQt
)
