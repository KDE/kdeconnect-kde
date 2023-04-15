if(WIN32)
    set(kdeconnect_pausemusic_SRCS
        pausemusicplugin-win.cpp
    )
else()
    set(kdeconnect_pausemusic_SRCS
        pausemusicplugin.cpp
    )
endif()

set(debug_file_SRCS)
ecm_qt_declare_logging_category(
    debug_file_SRCS HEADER plugin_pausemusic_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_PAUSEMUSIC CATEGORY_NAME kdeconnect.plugin.pausemusic
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin pausemusic)")

qt_add_dbus_interface(kdeconnect_pausemusic_SRCS org.mpris.MediaPlayer2.Player.xml mprisplayer)

kdeconnect_add_plugin(kdeconnect_pausemusic SOURCES ${kdeconnect_pausemusic_SRCS} ${debug_file_SRCS})

target_link_libraries(kdeconnect_pausemusic
    kdeconnectcore
    Qt::Core
    Qt::DBus
)
if (NOT WIN32)
    target_link_libraries(kdeconnect_pausemusic
        KF${QT_MAJOR_VERSION}::PulseAudioQt
    )
else()
    target_link_libraries(kdeconnect_pausemusic
        windowsapp
    )
    target_compile_features(kdeconnect_pausemusic PUBLIC cxx_std_17)
endif()

#######################################
# Config

set( kdeconnect_pausemusic_config_SRCS  )

kcoreaddons_add_plugin(kdeconnect_pausemusic_config SOURCES pausemusic_config.cpp INSTALL_NAMESPACE "kdeconnect/kcms")

ki18n_wrap_ui(kdeconnect_pausemusic_config pausemusic_config.ui)

target_link_libraries( kdeconnect_pausemusic_config
    kdeconnectcore
    kdeconnectpluginkcm
    KF${QT_MAJOR_VERSION}::I18n
    KF${QT_MAJOR_VERSION}::KCMUtils
)
