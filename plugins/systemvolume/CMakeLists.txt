if(WIN32)
    set(kdeconnect_systemvolume_SRCS
        systemvolumeplugin-win.cpp
    )
elseif(APPLE)
    set(kdeconnect_systemvolume_SRCS
        systemvolumeplugin-macos.cpp
    )
else()
    set(kdeconnect_systemvolume_SRCS
        systemvolumeplugin-pulse.cpp
    )
endif()

kdeconnect_add_plugin(kdeconnect_systemvolume JSON kdeconnect_systemvolume.json SOURCES ${kdeconnect_systemvolume_SRCS})

if(WIN32)
    target_link_libraries(kdeconnect_systemvolume
        kdeconnectcore
        Qt5::Core
        ole32
    )
elseif(APPLE)
    target_link_libraries(kdeconnect_systemvolume
        kdeconnectcore
        Qt5::Core
        "-framework CoreAudio"
    )
else()
    target_link_libraries(kdeconnect_systemvolume
        kdeconnectcore
        Qt5::Core
        KF5::PulseAudioQt
    )
endif()
