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

ecm_qt_declare_logging_category(kdeconnect_systemvolume
    HEADER plugin_systemvolume_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_SYSTEMVOLUME CATEGORY_NAME kdeconnect.plugin.systemvolume
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin systemvolume)")
if(WIN32)
    target_link_libraries(kdeconnect_systemvolume
        kdeconnectcore
        Qt::Core
        ole32
    )
    target_sources(kdeconnect_systemvolume
            PRIVATE
            PolicyConfig.h)
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
        KF${QT_MAJOR_VERSION}::PulseAudioQt
    )
endif()
