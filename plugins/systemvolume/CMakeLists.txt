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

kdeconnect_add_plugin(kdeconnect_systemvolume SOURCES ${kdeconnect_systemvolume_SRCS})

if(WIN32)
    target_link_libraries(kdeconnect_systemvolume
        kdeconnectcore
        Qt::Core
        ole32
    )
elseif(APPLE)
    target_link_libraries(kdeconnect_systemvolume
        kdeconnectcore
        Qt::Core
        "-framework CoreAudio"
        "-framework CoreFoundation"
    )
else()
    target_link_libraries(kdeconnect_systemvolume
        kdeconnectcore
        Qt::Core
        KF6::PulseAudioQt
    )
endif()
