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

set(debug_file_SRCS)
ecm_qt_declare_logging_category(
    debug_file_SRCS HEADER plugin_systemvolume_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_SYSTEMVOLUME CATEGORY_NAME kdeconnect.plugin.systemvolume
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin systemvolume)")
kdeconnect_add_plugin(kdeconnect_systemvolume SOURCES ${kdeconnect_systemvolume_SRCS} ${debug_file_SRCS})

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
    )
else()
    target_link_libraries(kdeconnect_systemvolume
        kdeconnectcore
        Qt::Core
        KF${QT_MAJOR_VERSION}::PulseAudioQt
    )
endif()
