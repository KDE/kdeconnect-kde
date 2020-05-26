set(debug_file_SRCS)
ecm_qt_declare_logging_category(
    debug_file_SRCS HEADER plugin_findthisdevice_debug.h
    IDENTIFIER KDECONNECT_PLUGIN_FINDTHISDEVICE CATEGORY_NAME kdeconnect.plugin.findthisdevice
    DEFAULT_SEVERITY Warning
    EXPORT kdeconnect-kde DESCRIPTION "kdeconnect (plugin findthisdevice)")

set(kdeconnect_findthisdevice_SRCS
    findthisdeviceplugin.cpp
    ${debug_file_SRCS}
)

kdeconnect_add_plugin(kdeconnect_findthisdevice
    JSON kdeconnect_findthisdevice.json
    SOURCES ${kdeconnect_findthisdevice_SRCS})

target_link_libraries(kdeconnect_findthisdevice
    kdeconnectcore
    Qt5::Core
    Qt5::Multimedia
    Qt5::DBus
)

if (NOT WIN32)
    target_link_libraries(kdeconnect_findthisdevice
        KF5::PulseAudioQt
    )
endif()

set(kdeconnect_findthisdevice_config_SRCS findthisdevice_config.cpp)
ki18n_wrap_ui(kdeconnect_findthisdevice_config_SRCS findthisdevice_config.ui)

add_library(kdeconnect_findthisdevice_config MODULE ${kdeconnect_findthisdevice_config_SRCS} ${debug_file_SRCS})
target_link_libraries(kdeconnect_findthisdevice_config
    kdeconnectpluginkcm
    Qt5::Multimedia
    KF5::I18n
    KF5::CoreAddons
    KF5::ConfigWidgets
    KF5::KIOWidgets     # KUrlRequester
)

install(TARGETS kdeconnect_findthisdevice_config DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES kdeconnect_findthisdevice_config.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
